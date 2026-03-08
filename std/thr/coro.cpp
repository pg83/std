#include "coro.h"
#include "task.h"
#include "pool.h"

#include <std/sys/crt.h>
#include <std/ptr/scoped.h>
#include <std/alg/destruct.h>

#include <ucontext.h>

using namespace stl;

namespace {
    struct CoroExecutorImpl;

    struct alignas(max_align_t) ContImpl: public Cont, public Task {
        CoroExecutorImpl* exec_;
        ucontext_t ctx_;
        ucontext_t* workerCtx_;
        Runable* runable_;

        ContImpl(CoroExecutorImpl* exec, SpawnParams params) noexcept;
        virtual ~ContImpl() = default;

        CoroExecutor* executor() noexcept override;
        void run() noexcept override;
        void entryX() noexcept;

        static void entry(u32 lo, u32 hi) noexcept;
    };

    struct HeapContImpl: public ContImpl {
        using ContImpl::ContImpl;

        ~HeapContImpl() override {
            freeMemory(ctx_.uc_stack.ss_sp);
        }
    };

    ContImpl* makeContImpl(CoroExecutorImpl* exec, SpawnParams params) {
        if (params.stackPtr) {
            return new ContImpl(exec, params);
        } else {
            return new HeapContImpl(exec, params.setStackPtr(allocateMemory(params.stackSize)));
        }
    }

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

        void spawnRun(SpawnParams params) override {
            pool_->submitTask(makeContImpl(this, params));
        }

        Cont* me() const noexcept override {
            return ((CoroExecutorImpl*)this)->currentCont();
        }

        void yield() noexcept override {
            auto* c = currentCont();

            swapcontext(&c->ctx_, c->workerCtx_);
        }

        ThreadPool* pool() const noexcept override {
            return (ThreadPool*)pool_.ptr();
        }
    };
}

ContImpl::ContImpl(CoroExecutorImpl* exec, SpawnParams params) noexcept
    : exec_(exec)
    , workerCtx_(nullptr)
    , runable_(params.runable)
{
    getcontext(&ctx_);

    ctx_.uc_stack.ss_sp = params.stackPtr;
    ctx_.uc_stack.ss_size = params.stackSize;
    ctx_.uc_link = nullptr;

    auto p = (uintptr_t)this;

    makecontext(&ctx_, (void (*)())ContImpl::entry, 2, (u32)p, (u32)(p >> 32));
}

SpawnParams::SpawnParams() noexcept
    : stackSize(16 * 1024)
    , stackPtr(nullptr)
    , runable(nullptr)
{
}

CoroExecutor* ContImpl::executor() noexcept {
    return exec_;
}

void ContImpl::entryX() noexcept {
    runable_->run();
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
        delete this;
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

SpawnParams& SpawnParams::setStackSize(size_t v) noexcept {
    stackSize = v;

    return *this;
}

SpawnParams& SpawnParams::setStackPtr(void* v) noexcept {
    stackPtr = v;

    return *this;
}

SpawnParams& SpawnParams::setRunablePtr(Runable* v) noexcept {
    runable = v;

    return *this;
}
