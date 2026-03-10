#include "coro.h"
#include "task.h"
#include "pool.h"
#include "mutex.h"
#include "cond_var.h"
#include "mutex_iface.h"
#include "cond_var_iface.h"
#include "channel_iface.h"
#include "thread_iface.h"
#include "poller.h"
#include "thread.h"

#include <std/sys/crt.h>
#include <std/sys/atomic.h>
#include <std/ptr/scoped.h>
#include <std/alg/destruct.h>
#include <std/lib/list.h>
#include <std/lib/vector.h>
#include <std/lib/ring_buf.h>
#include <std/dbg/insist.h>
#include <std/map/treap.h>
#include <std/map/treap_node.h>
#include <std/mem/obj_pool.h>
#include <std/rng/pcg.h>

#include <std/sys/fd.h>

#include <ucontext.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

using namespace stl;

namespace {
    struct CoroExecutorImpl;
    struct ReactorThread;

    static u64 monotonicNowNs() noexcept {
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
    }

    struct ContImpl: public Cont, public Task {
        CoroExecutorImpl* exec_;
        ucontext_t ctx_;
        ucontext_t* workerCtx_;
        Runable* runable_;
        Runable* afterSuspend_;
        PCG32 rng_;

        ContImpl(CoroExecutorImpl* exec, SpawnParams params) noexcept;

        virtual ~ContImpl() = default;

        void parkWith(Runable* afterSuspend) noexcept;
        CoroExecutor* executor() noexcept override;
        u32 poll(int fd, u32 flags) override;
        u32 poll(int fd, u32 flags, u32 timeoutMs) override;
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

        explicit CoroExecutorImpl(ThreadPool::Ref pool);
        ~CoroExecutorImpl() noexcept override;

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

        ReactorThread* pickReactor(PCG32& rng) noexcept;

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
        bool closed_;

        CoroChannelImpl(CoroExecutorImpl* exec) noexcept;

        bool sendOne(void* v) noexcept;
        bool recvOne(void** out) noexcept;
        void wakeWaiter(Waiter* w) noexcept;

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

    // -------------------------------------------------------------------------
    // Reactor: poll(fd, flags[, timeoutMs]) — non-blocking I/O for coroutines
    // -------------------------------------------------------------------------

    struct PollRequest: public TreapNode, public Runable {
        ContImpl* cont;
        ReactorThread* reactor;
        int fd;
        u32 flags;
        u32 result;   // readiness flags returned, 0 = timeout
        u64 deadline; // absolute monotonic ns

        void* key() const noexcept override {
            return (void*)this;
        }

        void run() override;  // afterSuspend: registers request with reactor
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
    constexpr u32 REACTOR_MAX_IDLE_MS = 30000;

    struct ReactorThread: public Runable {
        CoroExecutorImpl* exec;
        PollerIface* poller;
        DeadlineTreap timers;
        Mutex timerMutex;
        ScopedFD wakeReadFd;
        ScopedFD wakeWriteFd;
        Thread* thread_;
        bool done;

        ReactorThread(CoroExecutorImpl* e)
            : exec(e)
            , poller(PollerIface::create())
            , timerMutex()
            , thread_(nullptr)
            , done(false)
        {
            createPipeFD(wakeReadFd, wakeWriteFd);
            // set read end non-blocking so drainWakeup() can loop safely
            ::fcntl(wakeReadFd.get(), F_SETFL, O_NONBLOCK);
            poller->arm(wakeReadFd.get(), PollFlag::In, nullptr);
            thread_ = new Thread(*this);
        }

        ~ReactorThread() noexcept {
            delete thread_;
            delete poller;
        }

        void join() noexcept {
            done = true;
            wakeup();
            thread_->join();
        }

        void registerRequest(PollRequest* req) {
            u64 reqDeadline = req->deadline;
            u64 prevEarliest;

            {
                LockGuard g(timerMutex);
                prevEarliest = timers.earliest();
                timers.insert(req);
            }

            // arm AFTER insert: if fd fires immediately, reactor can safely
            // remove req from timers before the coroutine stack unwinds
            poller->arm(req->fd, req->flags, req);

            if (reqDeadline < prevEarliest) {
                wakeup();
            }
        }

        void wakeup() noexcept {
            char b = 1;
            ::write(wakeWriteFd.get(), &b, 1);
        }

        void drainWakeup() noexcept {
            char buf[64];
            while (::read(wakeReadFd.get(), buf, sizeof(buf)) > 0) {
            }
        }

        void run() noexcept override {
            while (!done) {
                // Compute wait timeout
                u64 now = monotonicNowNs();
                u64 earliest;
                {
                    LockGuard g(timerMutex);
                    earliest = timers.earliest();
                }

                u32 timeoutMs;
                if (earliest <= now) {
                    timeoutMs = 0;
                } else {
                    u64 diffMs = (earliest - now) / 1000000ULL;
                    timeoutMs = (u32)(diffMs < REACTOR_MAX_IDLE_MS ? diffMs : REACTOR_MAX_IDLE_MS);
                }

                PollEvent events[REACTOR_MAX_EVENTS];
                u32 n = poller->wait(events, REACTOR_MAX_EVENTS, timeoutMs);

                // Process fd-ready events
                for (u32 i = 0; i < n; i++) {
                    if (events[i].data == nullptr) {
                        // wakeup event — drain and re-arm
                        drainWakeup();
                        poller->arm(wakeReadFd.get(), PollFlag::In, nullptr);
                    } else {
                        auto* req = (PollRequest*)events[i].data;
                        req->result = events[i].flags;
                        {
                            LockGuard g(timerMutex);
                            timers.remove(req);
                        }
                        req->cont->afterSuspend_ = nullptr;
                        req->cont->reSchedule();
                    }
                }

                // Expire timers
                now = monotonicNowNs();
                LockGuard g(timerMutex);
                while (auto* node = timers.min()) {
                    auto* req = (PollRequest*)node;
                    if (req->deadline > now) {
                        break;
                    }
                    timers.remove(req);
                    poller->disarm(req->fd);
                    req->result = 0;
                    req->cont->afterSuspend_ = nullptr;
                    req->cont->reSchedule();
                }
            }
        }
    };

    void PollRequest::run() {
        reactor->registerRequest(this);
    }
}

CoroExecutorImpl::CoroExecutorImpl(ThreadPool::Ref pool)
    : pool_(pool)
    , tlsKey_(registerTlsKey())
    , reactorPool_(ObjPool::fromMemory())
    , reactors_()
{
    reactors_.pushBack(reactorPool_->make<ReactorThread>(this));
}

CoroExecutorImpl::~CoroExecutorImpl() noexcept {
    for (auto* r : reactors_) {
        r->join();
    }
}

ReactorThread* CoroExecutorImpl::pickReactor(PCG32& rng) noexcept {
    return reactors_[rng.uniformBiased((u32)reactors_.length())];
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
    , rng_(this)
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

u32 ContImpl::poll(int fd, u32 flags, u32 timeoutMs) {
    PollRequest req;
    req.cont     = this;
    req.reactor  = exec_->pickReactor(rng_);
    req.fd       = fd;
    req.flags    = flags;
    req.result   = 0;
    req.deadline = monotonicNowNs() + (u64)timeoutMs * 1000000ULL;

    parkWith(&req);

    return req.result;
}

u32 ContImpl::poll(int fd, u32 flags) {
    return poll(fd, flags, 86400000u);
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

CoroChannelImpl::CoroChannelImpl(CoroExecutorImpl* exec) noexcept
    : exec_(exec)
    , queueMutex_(exec)
    , closed_(false)
{
}

void CoroChannelImpl::wakeWaiter(Waiter* w) noexcept {
    w->cont->afterSuspend_ = nullptr;
    w->cont->reSchedule();
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
        wakeWaiter(w);

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
        wakeWaiter(w);

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
            wakeWaiter(w);
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
