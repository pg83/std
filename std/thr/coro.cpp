#include "coro.h"
#include "task.h"
#include "pool.h"
#include "mutex.h"
#include "mutex_iface.h"
#include "cond_var_iface.h"

#include <std/sys/crt.h>
#include <std/ptr/scoped.h>
#include <std/alg/destruct.h>
#include <std/lib/list.h>

#include <ucontext.h>

using namespace stl;

namespace {
    struct CoroExecutorImpl;

    struct ContImpl: public Cont, public Task {
        CoroExecutorImpl* exec_;
        ucontext_t ctx_;
        ucontext_t* workerCtx_;
        Runable* runable_;
        Runable* afterSuspend_;

        ContImpl(CoroExecutorImpl* exec, SpawnParams params) noexcept;

        virtual ~ContImpl() = default;

        CoroExecutor* executor() noexcept override;
        void reSchedule() noexcept;
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

    struct CoroMutexImpl;
    struct CoroCondVarImpl;

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

        MutexIface* createMutex() override;
        CondVarIface* createCondVar() override;
    };

    struct CoroCondVarImpl: public CondVarIface, public Runable {
        CoroExecutorImpl* exec_;
        Mutex queueMutex_;
        IntrusiveList waiters_;

        CoroCondVarImpl(CoroExecutorImpl* exec) noexcept;

        void run() override;
        void wait(MutexIface* mutex) noexcept override;
        void signal() noexcept override;
        void broadcast() noexcept override;
    };

    struct CoroMutexImpl: public MutexIface, public Runable {
        CoroExecutorImpl* exec_;
        Mutex queueMutex_;
        IntrusiveList waiters_;
        bool locked_;

        CoroMutexImpl(CoroExecutorImpl* exec) noexcept;

        void run() override;
        void lock() noexcept override;
        void unlock() noexcept override;
        bool tryLock() noexcept override;
    };
}

CoroMutexImpl::CoroMutexImpl(CoroExecutorImpl* exec) noexcept
    : exec_(exec)
    , locked_(false)
{
}

void CoroMutexImpl::run() {
    queueMutex_.unlock();
}

void CoroMutexImpl::lock() noexcept {
    auto* cont = exec_->currentCont();

    queueMutex_.lock();

    if (!locked_) {
        locked_ = true;
        queueMutex_.unlock();

        return;
    }

    waiters_.pushBack(cont);
    cont->afterSuspend_ = this;
    swapcontext(&cont->ctx_, cont->workerCtx_);
    // resumed here — worker already ran afterSuspend_
}

void CoroMutexImpl::unlock() noexcept {
    LockGuard guard(queueMutex_);

    if (auto* node = waiters_.popFrontOrNull(); node) {
        auto* cont = (ContImpl*)(Task*)node;

        cont->afterSuspend_ = nullptr;
        cont->reSchedule();
    } else {
        locked_ = false;
    }
}

bool CoroMutexImpl::tryLock() noexcept {
    LockGuard guard(queueMutex_);

    if (!locked_) {
        return locked_ = true;
    }

    return false;
}

ContImpl::ContImpl(CoroExecutorImpl* exec, SpawnParams params) noexcept
    : exec_(exec)
    , workerCtx_(nullptr)
    , runable_(params.runable)
    , afterSuspend_(nullptr)
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

void ContImpl::reSchedule() noexcept {
    exec_->pool_->submitTask(this);
}

void ContImpl::run() noexcept {
    ucontext_t workerCtx;

    *exec_->tls() = this;
    workerCtx_ = &workerCtx;
    swapcontext(&workerCtx, &ctx_);
    *exec_->tls() = nullptr;

    if (auto* as = afterSuspend_) {
        // after run(), cont may already be rescheduled by another thread
        return as->run();
    }

    if (runable_) {
        reSchedule();
    } else {
        delete this;
    }
}

MutexIface* CoroExecutorImpl::createMutex() {
    return new CoroMutexImpl(this);
}

CoroCondVarImpl::CoroCondVarImpl(CoroExecutorImpl* exec) noexcept
    : exec_(exec)
{
}

void CoroCondVarImpl::run() {
    queueMutex_.unlock();
}

void CoroCondVarImpl::wait(MutexIface* mutex) noexcept {
    auto* cont = exec_->currentCont();

    queueMutex_.lock();
    waiters_.pushBack(cont);
    cont->afterSuspend_ = this;
    mutex->unlock();
    swapcontext(&cont->ctx_, cont->workerCtx_);
    // resumed — queueMutex_ was unlocked by run(), mutex is not held
    mutex->lock();
}

void CoroCondVarImpl::signal() noexcept {
    LockGuard guard(queueMutex_);

    if (auto* node = waiters_.popFrontOrNull(); node) {
        auto* cont = (ContImpl*)(Task*)node;

        cont->afterSuspend_ = nullptr;
        cont->reSchedule();
    }
}

void CoroCondVarImpl::broadcast() noexcept {
    LockGuard guard(queueMutex_);

    while (auto* node = waiters_.popFrontOrNull()) {
        auto* cont = (ContImpl*)(Task*)node;

        cont->afterSuspend_ = nullptr;
        cont->reSchedule();
    }
}

CondVarIface* CoroExecutorImpl::createCondVar() {
    return new CoroCondVarImpl(this);
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
