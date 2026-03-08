#include "coro.h"
#include "task.h"
#include "pool.h"

#include <std/mem/new.h>
#include <std/sys/crt.h>
#include <std/ptr/scoped.h>
#include <std/alg/destruct.h>

#include <ucontext.h>

using namespace stl;

namespace {
    static constexpr size_t STACK_SIZE = 64 * 1024;

    struct CoroExecutorImpl;

    struct alignas(max_align_t) ContImpl: public Cont, public Task, public Newable {
        CoroExecutorImpl* exec_;
        ucontext_t ctx_;
        ucontext_t* workerCtx_;
        Runable* runable_;

        ContImpl(CoroExecutorImpl* exec, Runable* runable) noexcept;

        CoroExecutor* executor() noexcept override;
        void run() noexcept override;
        void entryX() noexcept;

        static void entry(u32 lo, u32 hi) noexcept;
    };

    struct CoroExecutorImpl: public CoroExecutor {
        ThreadPool::Ref pool_;
        const u64 tlsKey_;

        explicit CoroExecutorImpl(ThreadPool::Ref pool) noexcept
            : pool_(pool)
            , tlsKey_(registerTlsKey())
        {
        }

        auto tls() {
            return pool_->tls(tlsKey_);
        }

        ContImpl* currentCont() {
            return (ContImpl*)*tls();
        }

        void spawnRun(Runable* runable) override {
            pool_->submitTask(new (allocateMemory(STACK_SIZE)) ContImpl(this, runable));
        }

        Cont* me() const noexcept override {
            return ((CoroExecutorImpl*)this)->currentCont();
        }

        void yield() noexcept override {
            auto* c = currentCont();

            swapcontext(&c->ctx_, c->workerCtx_);
        }

        ThreadPool* pool() const noexcept {
            return (ThreadPool*)pool_.ptr();
        }
    };
}

ContImpl::ContImpl(CoroExecutorImpl* exec, Runable* runable) noexcept
    : exec_(exec)
    , workerCtx_(nullptr)
    , runable_(runable)
{
    getcontext(&ctx_);

    ctx_.uc_stack.ss_sp = (char*)(this + 1);
    ctx_.uc_stack.ss_size = STACK_SIZE - sizeof(ContImpl);
    ctx_.uc_link = nullptr;

    auto p = (uintptr_t)this;

    makecontext(&ctx_, (void (*)())ContImpl::entry, 2, (u32)p, (u32)(p >> 32));
}

CoroExecutor* ContImpl::executor() noexcept {
    return exec_;
}

void ContImpl::entryX() noexcept {
    runable_->run();
    delete runable_;
    runable_ = nullptr;
    swapcontext(&ctx_, workerCtx_);
}

void ContImpl::entry(u32 lo, u32 hi) noexcept {
    ((ContImpl*)(((uintptr_t)hi << 32) | lo))->entryX();
}

void ContImpl::run() noexcept {
    ucontext_t workerCtx;

    workerCtx_ = &workerCtx;

    *exec_->tls() = this;

    swapcontext(&workerCtx, &ctx_);

    *exec_->tls() = nullptr;

    if (!runable_) {
        freeMemory(destruct(this));
    } else {
        exec_->pool_->submitTask(this);
    }
}

CoroExecutor::~CoroExecutor() noexcept {
}

CoroExecutor::Ref CoroExecutor::create(ThreadPool* pool) {
    return new CoroExecutorImpl(pool);
}

CoroExecutor::Ref CoroExecutor::create(size_t threads) {
    return create(ThreadPool::workStealing(threads).mutPtr());
}
