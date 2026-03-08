#include "coro.h"
#include "task.h"

#include <std/mem/new.h>

#include <stdlib.h>
#include <ucontext.h>

using namespace stl;

namespace {
    static constexpr size_t STACK_SIZE = 64 * 1024;

    struct CoroExecutorImpl;

    struct ContImpl: public Cont, public Task, public Newable {
        CoroExecutorImpl* exec_;
        ucontext_t ctx_;
        ucontext_t* workerCtx_;
        bool done_;
        void (*fn_)(Cont*, void*);
        void* fnCtx_;

        ContImpl(CoroExecutorImpl* exec, void (*fn)(Cont*, void*), void* fnCtx) noexcept;

        CoroExecutor* executor() noexcept override;
        void run() noexcept override;

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

        ContImpl* currentCont() noexcept {
            return (ContImpl*)*pool_->tls(tlsKey_);
        }

        void spawn(void (*fn)(Cont*, void*), void* ctx) noexcept override {
            pool_->submitTask(new (malloc(STACK_SIZE)) ContImpl(this, fn, ctx));
        }

        void yield() noexcept override {
            auto* c = currentCont();
            swapcontext(&c->ctx_, c->workerCtx_);
        }
    };

    ContImpl::ContImpl(CoroExecutorImpl* exec, void (*fn)(Cont*, void*), void* fnCtx) noexcept
        : exec_(exec)
        , workerCtx_(nullptr)
        , done_(false)
        , fn_(fn)
        , fnCtx_(fnCtx)
    {
        getcontext(&ctx_);

        ctx_.uc_stack.ss_sp = (char*)this + sizeof(ContImpl);
        ctx_.uc_stack.ss_size = STACK_SIZE - sizeof(ContImpl);
        ctx_.uc_link = nullptr;

        auto p = (uintptr_t)this;
        makecontext(&ctx_, (void (*)())ContImpl::entry, 2, (u32)p, (u32)(p >> 32));
    }

    CoroExecutor* ContImpl::executor() noexcept {
        return exec_;
    }

    void ContImpl::entry(u32 lo, u32 hi) noexcept {
        auto* c = (ContImpl*)(((uintptr_t)hi << 32) | lo);

        c->fn_(c, c->fnCtx_);
        c->done_ = true;
        swapcontext(&c->ctx_, c->workerCtx_);
    }

    void ContImpl::run() noexcept {
        ucontext_t workerCtx;

        workerCtx_ = &workerCtx;
        *exec_->pool_->tls(exec_->tlsKey_) = this;

        swapcontext(&workerCtx, &ctx_);

        if (done_) {
            this->~ContImpl();
            free(this);
        } else {
            exec_->pool_->submitTask(this);
        }
    }
}

CoroExecutor::~CoroExecutor() noexcept {
}

CoroExecutor::Ref CoroExecutor::create(ThreadPool::Ref pool) {
    return new CoroExecutorImpl(pool);
}
