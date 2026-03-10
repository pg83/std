#include "coro.h"
#include "task.h"
#include "pool.h"
#include "mutex.h"
#include "cond_var.h"
#include "mutex_iface.h"
#include "cond_var_iface.h"
#include "channel_iface.h"
#include "thread_iface.h"

#include <std/sys/crt.h>
#include <std/ptr/scoped.h>
#include <std/alg/destruct.h>
#include <std/lib/list.h>
#include <std/lib/ring_buf.h>
#include <std/dbg/insist.h>

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

        void parkWith(Runable* afterSuspend) noexcept;
        CoroExecutor* executor() noexcept override;
        void run() noexcept override;
        void reSchedule() noexcept;
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
    struct CoroChannelImpl;

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
            currentCont()->parkWith(nullptr);
        }

        void parkWith(Runable* afterSuspend) noexcept {
            currentCont()->parkWith(afterSuspend);
        }

        ThreadPool* pool() const noexcept override {
            return (ThreadPool*)pool_.ptr();
        }

        MutexIface* createMutex() override;
        CondVarIface* createCondVar() override;
        ChannelIface* createChannel(size_t cap) override;
        ThreadIface* createThread(Runable& runable) override;
    };

    struct CoroThreadImpl: public ThreadIface {
        CoroExecutorImpl* exec_;
        Runable* runable_;
        Mutex mtx_;
        CondVar cv_;
        bool finished_ = false;

        CoroThreadImpl(CoroExecutorImpl* exec, Runable& runable);

        void start() override;
        void notifyDone() noexcept;
        void join() noexcept override;
        void detach() noexcept override;
        u64 threadId() const noexcept override;
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

    struct Waiter: public IntrusiveNode {
        ContImpl* cont;
        void* value;
        bool valueSet;
    };

    struct CoroChannelImpl: public ChannelIface, public Runable {
        CoroExecutorImpl* exec_;
        Mutex queueMutex_;
        IntrusiveList senders_;
        IntrusiveList receivers_;
        RingBuffer buf_;
        bool closed_;

        CoroChannelImpl(CoroExecutorImpl* exec, size_t capacity) noexcept;

        void* operator new(size_t, void* p) noexcept {
            return p;
        }
        void operator delete(void* p) noexcept {
            freeMemory(p);
        }

        void wakeWaiter(Waiter* w) noexcept;
        bool sendOne(void* v) noexcept;
        bool recvOne(void** out) noexcept;

        void run() override;
        void enqueue(void* v) override;
        bool dequeue(void** out) override;
        bool tryEnqueue(void* v) override;
        bool tryDequeue(void** out) override;
        void close() override;
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
    cont->parkWith(this);
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

void ContImpl::parkWith(Runable* afterSuspend) noexcept {
    afterSuspend_ = afterSuspend;
    swapcontext(&ctx_, workerCtx_);
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
    mutex->unlock();
    cont->parkWith(this);
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

CoroThreadImpl::CoroThreadImpl(CoroExecutorImpl* exec, Runable& runable)
    : exec_(exec)
    , runable_(&runable)
    , mtx_(exec)
    , cv_(exec)
{
}

void CoroThreadImpl::start() {
    exec_->spawnRun(SpawnParams().setRunable([this]() {
        runable_->run();
        notifyDone();
    }));
}

void CoroThreadImpl::notifyDone() noexcept {
    LockGuard guard(mtx_);

    finished_ = true;
    cv_.signal();
}

void CoroThreadImpl::join() noexcept {
    LockGuard guard(mtx_);

    while (!finished_) {
        cv_.wait(mtx_);
    }
}

void CoroThreadImpl::detach() noexcept {
}

u64 CoroThreadImpl::threadId() const noexcept {
    return (size_t)this;
}

ThreadIface* CoroExecutorImpl::createThread(Runable& runable) {
    return new CoroThreadImpl(this, runable);
}

CoroChannelImpl::CoroChannelImpl(CoroExecutorImpl* exec, size_t capacity) noexcept
    : exec_(exec)
    , queueMutex_(exec)
    , buf_((void**)(this + 1), capacity)
    , closed_(false)
{
}

void CoroChannelImpl::wakeWaiter(Waiter* w) noexcept {
    w->cont->afterSuspend_ = nullptr;
    w->cont->reSchedule();
}

// Called under lock. Returns true if sent, false if no room. Lock is NOT released.
bool CoroChannelImpl::sendOne(void* v) noexcept {
    if (!receivers_.empty()) {
        auto* w = (Waiter*)receivers_.popFront();

        w->value = v;
        w->valueSet = true;
        wakeWaiter(w);

        return true;
    }

    if (!buf_.full()) {
        buf_.push(v);

        return true;
    }

    return false;
}

// Called under lock. Returns true if received, false if no data. Lock is NOT released.
bool CoroChannelImpl::recvOne(void** out) noexcept {
    if (!buf_.empty()) {
        *out = buf_.pop();

        if (!senders_.empty()) {
            auto* w = (Waiter*)senders_.popFront();

            buf_.push(w->value);
            wakeWaiter(w);
        }

        return true;
    }

    if (!senders_.empty()) {
        auto* w = (Waiter*)senders_.popFront();

        *out = w->value;
        wakeWaiter(w);

        return true;
    }

    return false;
}

void CoroChannelImpl::run() {
    queueMutex_.unlock();
}

void CoroChannelImpl::enqueue(void* v) {
    auto* cont = exec_->currentCont();

    LockGuard guard(queueMutex_);

    STD_INSIST(!closed_);

    if (sendOne(v)) {
        return;
    }

    Waiter w;

    w.cont = cont;
    w.value = v;
    w.valueSet = true;

    senders_.pushBack(&w);
    guard.drop();
    cont->parkWith(this);
}

bool CoroChannelImpl::dequeue(void** out) {
    auto* cont = exec_->currentCont();

    LockGuard guard(queueMutex_);

    if (recvOne(out)) {
        return true;
    }

    if (closed_) {
        return false;
    }

    Waiter w;

    w.cont = cont;
    w.value = nullptr;
    w.valueSet = false;

    receivers_.pushBack(&w);
    guard.drop();
    cont->parkWith(this);

    if (w.valueSet) {
        *out = w.value;

        return true;
    }

    return false;
}

bool CoroChannelImpl::tryEnqueue(void* v) {
    LockGuard guard(queueMutex_);

    STD_INSIST(!closed_);

    return sendOne(v);
}

bool CoroChannelImpl::tryDequeue(void** out) {
    LockGuard guard(queueMutex_);

    return recvOne(out);
}

void CoroChannelImpl::close() {
    LockGuard guard(queueMutex_);

    STD_INSIST(!closed_);
    STD_INSIST(senders_.empty());

    closed_ = true;

    while (!receivers_.empty()) {
        wakeWaiter((Waiter*)receivers_.popFront());
    }
}

ChannelIface* CoroExecutorImpl::createChannel(size_t cap) {
    return new (allocateMemory(sizeof(CoroChannelImpl) + cap * sizeof(void*))) CoroChannelImpl(this, cap);
}

ChannelIface::~ChannelIface() noexcept {
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
