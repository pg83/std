#include "coro.h"
#include "task.h"
#include "pool.h"

#include <std/mem/new.h>
#include <std/sys/crt.h>
#include <std/alg/destruct.h>

#include <ucontext.h>

using namespace stl;

namespace {
    static constexpr size_t STACK_SIZE = 64 * 1024;

    struct CoroExecutorImpl;

    struct ContImpl: public Cont, public Task, public Newable {
        CoroExecutorImpl* exec_;
        ucontext_t ctx_;
        ucontext_t* workerCtx_;
        coro* fn_;
        void* fnCtx_;

        ContImpl(CoroExecutorImpl* exec, coro* fn, void* fnCtx) noexcept;

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

        void spawnCoro(coro* fn, void* ctx) override {
            pool_->submitTask(new (allocateMemory(STACK_SIZE)) ContImpl(this, fn, ctx));
        }

        Cont* me() const noexcept override {
            return ((CoroExecutorImpl*)this)->currentCont();
        }

        void yield() noexcept override {
            auto* c = currentCont();

            swapcontext(&c->ctx_, c->workerCtx_);
        }
    };
}

ContImpl::ContImpl(CoroExecutorImpl* exec, coro* fn, void* fnCtx) noexcept
    : exec_(exec)
    , workerCtx_(nullptr)
    , fn_(fn)
    , fnCtx_(fnCtx)
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
    fn_(this, fnCtx_);
    fn_ = nullptr;
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

    if (!fn_) {
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
