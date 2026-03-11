#include "coro.h"
#include "task.h"
#include "pool.h"
#include "mutex.h"
#include "poller.h"
#include "thread.h"
#include "cond_var.h"
#include "mutex_iface.h"
#include "thread_iface.h"
#include "channel_iface.h"
#include "cond_var_iface.h"

#include <std/sys/crt.h>
#include <std/rng/pcg.h>
#include <std/lib/list.h>
#include <std/map/treap.h>
#include <std/sys/atomic.h>
#include <std/ptr/scoped.h>
#include <std/lib/vector.h>
#include <std/dbg/insist.h>
#include <std/alg/destruct.h>
#include <std/lib/ring_buf.h>
#include <std/mem/obj_pool.h>
#include <std/map/treap_node.h>

#include <std/sys/fd.h>

#include <time.h>
#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <ucontext.h>

using namespace stl;

namespace {
    struct CoroExecutorImpl;
    struct ReactorThread;

    static u64 monotonicNowUs() noexcept {
        timespec ts;

        clock_gettime(CLOCK_MONOTONIC, &ts);

        return (u64)ts.tv_sec * 1000000ULL + (u64)ts.tv_nsec / 1000;
    }

    static size_t defaultReactors(size_t threads) noexcept {
        size_t r = threads / 4;

        if (r > 16) {
            r = 16;
        }

        if (r < 1) {
            r = 1;
        }

        return r;
    }

    struct ContImpl: public Cont, public Task {
        CoroExecutorImpl* exec_;
        ucontext_t ctx_;
        ucontext_t* workerCtx_;
        Runable* runable_;
        Runable* afterSuspend_;

        ContImpl(CoroExecutorImpl* exec, SpawnParams params) noexcept;

        virtual ~ContImpl();

        void parkWith(Runable* afterSuspend) noexcept;
        CoroExecutor* executor() noexcept override;
        void run() noexcept override;
        void reSchedule() noexcept;
        void entryX() noexcept;

        static void entry(u32 lo, u32 hi) noexcept;
    };

    struct alignas(max_align_t) HeapContImpl: public ContImpl {
        void* operator new(size_t, size_t stackSize) {
            return allocateMemory(sizeof(HeapContImpl) + stackSize);
        }

        void operator delete(void* p) noexcept {
            freeMemory(p);
        }

        HeapContImpl(CoroExecutorImpl* exec, SpawnParams params) noexcept
            : ContImpl(exec, params.setStackPtr((char*)(this + 1)))
        {
        }
    };

    ContImpl* makeContImpl(CoroExecutorImpl* exec, SpawnParams params) {
        if (params.stackPtr) {
            return new ContImpl(exec, params);
        } else {
            return new (params.stackSize) HeapContImpl(exec, params);
        }
    }

    struct CoroMutexImpl;
    struct CoroCondVarImpl;
    struct CoroChannelImpl;
    struct CoroChannelImplN;

    struct CoroExecutorImpl: public CoroExecutor {
        ThreadPool::Ref pool_;
        const u64 tlsKey_;
        ObjPool::Ref reactorPool_;
        Vector<ReactorThread*> reactors_;
        alignas(64) int inflight_ = 0;

        CoroExecutorImpl(ThreadPool::Ref pool, size_t reactors);
        ~CoroExecutorImpl() noexcept override;

        void join() noexcept override;

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

        u32 random() noexcept override {
            return pool_->random().nextU32();
        }

        void parkWith(Runable* afterSuspend) noexcept {
            currentCont()->parkWith(afterSuspend);
        }

        ThreadPool* pool() const noexcept override {
            return (ThreadPool*)pool_.ptr();
        }

        ReactorThread* pickReactor() noexcept;

        MutexIface* createMutex() override;
        CondVarIface* createCondVar() override;
        ChannelIface* createChannel(size_t cap) override;
        ThreadIface* createThread(Runable& runable) override;

        u32 poll(int fd, u32 flags, u64 timeoutUs) override;
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
        bool closed_;

        CoroChannelImpl(CoroExecutorImpl* exec) noexcept;

        bool sendOne(void* v) noexcept;
        bool recvOne(void** out) noexcept;

        virtual bool bufferOne(void* v) noexcept;
        virtual bool unbufferOne(void** out) noexcept;

        void run() override;
        void enqueue(void* v) override;
        bool dequeue(void** out) override;
        bool tryEnqueue(void* v) override;
        bool tryDequeue(void** out) override;
        void close() override;
    };

    struct CoroChannelImplN: public CoroChannelImpl {
        RingBuffer buf_;

        CoroChannelImplN(CoroExecutorImpl* exec, size_t capacity) noexcept;

        void* operator new(size_t, void* p) noexcept {
            return p;
        }

        void operator delete(void* p) noexcept {
            freeMemory(p);
        }

        bool bufferOne(void* v) noexcept override;
        bool unbufferOne(void** out) noexcept override;
    };

    struct CoroChannelImpl0: public CoroChannelImpl {
        CoroChannelImpl0(CoroExecutorImpl* exec) noexcept;
    };

    struct PollRequest: public TreapNode, public Runable {
        ContImpl* cont;
        ReactorThread* reactor;
        int fd;
        u32 flags;
        u32 result;   // readiness flags returned, 0 = timeout
        u64 deadline; // absolute monotonic us

        void* key() const noexcept override {
            return (void*)this;
        }

        void run() override; // afterSuspend: registers request with reactor
    };

    struct DeadlineTreap: public Treap {
        bool cmp(void* a, void* b) const noexcept override {
            auto* ra = (PollRequest*)a;
            auto* rb = (PollRequest*)b;

            if (ra->deadline != rb->deadline) {
                return ra->deadline < rb->deadline;
            }

            return ra < rb;
        }

        u64 earliest() const noexcept {
            auto* n = min();

            return n ? ((PollRequest*)n)->deadline : UINT64_MAX;
        }
    };

    constexpr u32 REACTOR_MAX_EVENTS = 64;
    constexpr u32 REACTOR_MAX_IDLE_US = 30000000;

    struct ReactorThread: public Runable {
        CoroExecutorImpl* exec;
        ScopedPtr<PollerIface> poller;
        DeadlineTreap timers;
        Mutex timerMutex;
        ScopedFD wakeReadFd;
        ScopedFD wakeWriteFd;
        ScopedPtr<Thread> thread_;
        bool done;

        ReactorThread(CoroExecutorImpl* e);

        ~ReactorThread() noexcept;

        void join() noexcept;
        void wakeup() noexcept;
        void drainWakeup() noexcept;
        void run() noexcept override;
        void registerRequest(PollRequest* req);
    };
}

void PollRequest::run() {
    reactor->registerRequest(this);
}

ContImpl::~ContImpl() {
    stdAtomicAddAndFetch(&exec_->inflight_, -1, MemoryOrder::Release);
}

ReactorThread::~ReactorThread() noexcept {
}

ReactorThread::ReactorThread(CoroExecutorImpl* e)
    : exec(e)
    , poller{PollerIface::create()}
    , thread_{}
    , done(false)
{
    createPipeFD(wakeReadFd, wakeWriteFd);
    // set read end non-blocking so drainWakeup() can loop safely
    ::fcntl(wakeReadFd.get(), F_SETFL, O_NONBLOCK);
    poller.ptr->arm(wakeReadFd.get(), PollFlag::In, nullptr);
    thread_.ptr = new Thread(*this);
}

void ReactorThread::join() noexcept {
    done = true;
    wakeup();
    thread_.ptr->join();
}

void ReactorThread::registerRequest(PollRequest* req) {
    u64 reqDeadline = req->deadline;
    u64 prevEarliest;

    {
        LockGuard g(timerMutex);
        prevEarliest = timers.earliest();
        timers.insert(req);
    }

    // arm AFTER insert: if fd fires immediately, reactor can safely
    // remove req from timers before the coroutine stack unwinds
    poller.ptr->arm(req->fd, req->flags, req);

    if (reqDeadline < prevEarliest) {
        wakeup();
    }
}

void ReactorThread::wakeup() noexcept {
    char b = 1;

    ::write(wakeWriteFd.get(), &b, 1);
}

void ReactorThread::drainWakeup() noexcept {
    char buf[64];

    while (::read(wakeReadFd.get(), buf, sizeof(buf)) > 0) {
    }
}

void ReactorThread::run() noexcept {
    while (!done) {
        // Compute wait timeout
        u64 now = monotonicNowUs();
        u64 earliest;

        {
            LockGuard g(timerMutex);

            earliest = timers.earliest();
        }

        u32 timeoutUs;

        if (earliest <= now) {
            timeoutUs = 0;
        } else {
            u64 diffUs = earliest - now;
            timeoutUs = (u32)(diffUs < REACTOR_MAX_IDLE_US ? diffUs : REACTOR_MAX_IDLE_US);
        }

        poller.ptr->wait([this](PollEvent* ev) {
            if (ev->data == nullptr) {
                // wakeup event — drain and re-arm
                drainWakeup();
                poller.ptr->arm(wakeReadFd.get(), PollFlag::In, nullptr);
            } else {
                auto* req = (PollRequest*)ev->data;

                req->result = ev->flags;

                {
                    LockGuard g(timerMutex);

                    timers.remove(req);
                }

                req->cont->reSchedule();
            }
        }, timeoutUs);

        // Expire timers
        now = monotonicNowUs();

        LockGuard g(timerMutex);

        while (auto* node = timers.min()) {
            auto* req = (PollRequest*)node;

            if (req->deadline > now) {
                break;
            }

            timers.remove(req);
            poller.ptr->disarm(req->fd);

            req->result = 0;
            req->cont->reSchedule();
        }
    }
}

CoroExecutorImpl::CoroExecutorImpl(ThreadPool::Ref pool, size_t reactors)
    : pool_(pool)
    , tlsKey_(registerTlsKey())
    , reactorPool_(ObjPool::fromMemory())
{
    for (size_t i = 0; i < reactors; ++i) {
        reactors_.pushBack(reactorPool_->make<ReactorThread>(this));
    }
}

CoroExecutorImpl::~CoroExecutorImpl() noexcept {
    join();

    for (auto* r : reactors_) {
        r->join();
    }
}

void CoroExecutorImpl::join() noexcept {
    while (stdAtomicFetch(&inflight_, MemoryOrder::Acquire) != 0) {
        sched_yield();
    }
}

ReactorThread* CoroExecutorImpl::pickReactor() noexcept {
    return reactors_[pool_->random().uniformBiased((u32)reactors_.length())];
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
    stdAtomicAddAndFetch(&exec_->inflight_, 1, MemoryOrder::Relaxed);
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

u32 CoroExecutorImpl::poll(int fd, u32 flags, u64 timeoutUs) {
    PollRequest req;

    req.cont = currentCont();
    req.reactor = pickReactor();
    req.fd = fd;
    req.flags = flags;
    req.result = 0;
    req.deadline = monotonicNowUs() + timeoutUs;

    req.cont->parkWith(&req);

    return req.result;
}

u32 Cont::poll(int fd, u32 flags, u64 timeoutUs) {
    return executor()->poll(fd, flags, timeoutUs);
}

u32 Cont::poll(int fd, u32 flags) {
    for (;;) {
        if (auto res = poll(fd, flags, REACTOR_MAX_IDLE_US); res) {
            return res;
        }
    }
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
        afterSuspend_ = nullptr;
        // after as->run(), cont may already be rescheduled by another thread
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

        cont->reSchedule();
    }
}

void CoroCondVarImpl::broadcast() noexcept {
    LockGuard guard(queueMutex_);

    while (auto* node = waiters_.popFrontOrNull()) {
        auto* cont = (ContImpl*)(Task*)node;

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
    exec_->spawn([this](Cont*) {
        join();
        delete this;
    });
}

u64 CoroThreadImpl::threadId() const noexcept {
    return (size_t)this;
}

ThreadIface* CoroExecutorImpl::createThread(Runable& runable) {
    return new CoroThreadImpl(this, runable);
}

CoroChannelImpl::CoroChannelImpl(CoroExecutorImpl* exec) noexcept
    : exec_(exec)
    , queueMutex_(exec)
    , closed_(false)
{
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
        ((Waiter*)receivers_.popFront())->cont->reSchedule();
    }
}

// CoroChannelImplN — buffered channel (cap > 0)

CoroChannelImplN::CoroChannelImplN(CoroExecutorImpl* exec, size_t capacity) noexcept
    : CoroChannelImpl(exec)
    , buf_((void**)(this + 1), capacity)
{
}

bool CoroChannelImpl::sendOne(void* v) noexcept {
    if (!receivers_.empty()) {
        auto* w = (Waiter*)receivers_.popFront();

        w->value = v;
        w->valueSet = true;
        w->cont->reSchedule();

        return true;
    }

    return bufferOne(v);
}

bool CoroChannelImpl::bufferOne(void*) noexcept {
    return false;
}

bool CoroChannelImpl::recvOne(void** out) noexcept {
    if (unbufferOne(out)) {
        return true;
    }

    if (!senders_.empty()) {
        auto* w = (Waiter*)senders_.popFront();

        *out = w->value;
        w->cont->reSchedule();

        return true;
    }

    return false;
}

bool CoroChannelImpl::unbufferOne(void**) noexcept {
    return false;
}

bool CoroChannelImplN::bufferOne(void* v) noexcept {
    if (!buf_.full()) {
        buf_.push(v);

        return true;
    }

    return false;
}

bool CoroChannelImplN::unbufferOne(void** out) noexcept {
    if (!buf_.empty()) {
        *out = buf_.pop();

        if (!senders_.empty()) {
            auto* w = (Waiter*)senders_.popFront();

            buf_.push(w->value);
            w->cont->reSchedule();
        }

        return true;
    }

    return false;
}

ChannelIface* CoroExecutorImpl::createChannel(size_t cap) {
    if (cap == 0) {
        return new CoroChannelImpl(this);
    }

    return new (allocateMemory(sizeof(CoroChannelImplN) + cap * sizeof(void*))) CoroChannelImplN(this, cap);
}

ChannelIface::~ChannelIface() noexcept {
}

CoroExecutor::~CoroExecutor() noexcept {
}

CoroExecutor::Ref CoroExecutor::create(ThreadPool* pool) {
    return create(pool, defaultReactors(pool->numThreads()));
}

CoroExecutor::Ref CoroExecutor::create(ThreadPool* pool, size_t reactors) {
    return new CoroExecutorImpl(pool, reactors);
}

CoroExecutor::Ref CoroExecutor::create(size_t threads) {
    return create(ThreadPool::workStealing(threads).mutPtr());
}

CoroExecutor::Ref CoroExecutor::create(size_t threads, size_t reactors) {
    return create(ThreadPool::workStealing(threads).mutPtr(), reactors);
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
