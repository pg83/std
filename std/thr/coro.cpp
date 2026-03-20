#include "coro.h"
#include "task.h"
#include "pool.h"
#include "mutex.h"
#include "guard.h"
#include "thread.h"
#include "poller.h"
#include "context.h"
#include "reactor.h"
#include "cond_var.h"
#include "semaphore.h"
#include "wait_group.h"
#include "thread_iface.h"
#include "channel_iface.h"
#include "cond_var_iface.h"
#include "semaphore_iface.h"

#include <std/sys/fd.h>
#include <std/sys/crt.h>
#include <std/rng/pcg.h>
#include <std/lib/list.h>
#include <std/alg/defer.h>
#include <std/alg/range.h>
#include <std/sys/atomic.h>
#include <std/lib/vector.h>
#include <std/dbg/insist.h>
#include <std/ptr/scoped.h>
#include <std/alg/minmax.h>
#include <std/dbg/assert.h>
#include <std/alg/exchange.h>
#include <std/alg/destruct.h>
#include <std/lib/ring_buf.h>
#include <std/mem/obj_pool.h>
#include <std/rng/split_mix_64.h>

#include <time.h>
#include <fcntl.h>
#include <sched.h>
#include <alloca.h>
#include <unistd.h>

using namespace stl;

namespace {
    struct CoroExecutorImpl;

    struct ContImpl: public Cont, public Task {
        CoroExecutorImpl* exec_;
        ScopedPtr<Context> ctx_;
        Context* workerCtx_;
        Runable* runable_;
        u8 priority_;

        ContImpl(CoroExecutorImpl* exec, void* ctxBuf, SpawnParams params) noexcept;

        virtual ~ContImpl() = default;

        void operator delete(void* p) noexcept {
            freeMemory(p);
        }

        void parkWith(Runable* afterSuspend) noexcept;
        u8 priority() const noexcept override;
        void run() noexcept override;
        void reSchedule() noexcept;
    };

    struct UserContImpl: public ContImpl {
        UserContImpl(CoroExecutorImpl* exec, void* ctxBuf, SpawnParams params) noexcept;
        ~UserContImpl();
    };

    template <typename Base>
    struct alignas(max_align_t) HeapContImpl: public Base {
        void* operator new(size_t, size_t stackSize) {
            return allocateMemory(sizeof(HeapContImpl) + Context::implSize() + stackSize);
        }

        HeapContImpl(CoroExecutorImpl* exec, SpawnParams params) noexcept
            : Base(exec, (char*)(this + 1), params.setStackPtr((char*)(this + 1) + Context::implSize()))
        {
        }
    };

    template <typename Base>
    struct alignas(max_align_t) ExtStackContImpl: public Base {
        void* operator new(size_t sz) {
            return allocateMemory(sz + Context::implSize());
        }

        ExtStackContImpl(CoroExecutorImpl* exec, SpawnParams params) noexcept
            : Base(exec, (char*)(this + 1), params)
        {
        }
    };

    template <typename Base>
    ContImpl* makeContImplT(CoroExecutorImpl* exec, SpawnParams params) {
        if (params.stackPtr) {
            return new ExtStackContImpl<Base>(exec, params);
        } else {
            return new (params.stackSize) HeapContImpl<Base>(exec, params);
        }
    }

    ContImpl* makeContImpl(CoroExecutorImpl* exec, SpawnParams params) {
        if (params.system) {
            return makeContImplT<ContImpl>(exec, params);
        } else {
            return makeContImplT<UserContImpl>(exec, params);
        }
    }

    struct CoroExecutorImpl: public CoroExecutor {
        alignas(64) int inflight_ = 0;
        ObjPool::Ref opool_;
        const u64 tlsKey_;
        Vector<ReactorIface*> reactors_;
        ScopedFD joinR_;
        ScopedFD joinW_;
        ScopedFD submitR_;
        ScopedFD submitW_;
        WaitGroup done_;
        ThreadPool* pool_;

        CoroExecutorImpl(size_t threads, size_t reactors);
        ~CoroExecutorImpl() noexcept override;

        void spawnSystem() noexcept;
        void join() noexcept override;
        void submitterLoop() noexcept;
        Cont* spawnRun(SpawnParams params) override;
        void submitExternalTask(Task* task) noexcept;

        auto tls() {
            return pool_->tls(tlsKey_);
        }

        ContImpl* currentCont() {
            return (ContImpl*)*tls();
        }

        Cont* me() const noexcept override {
            if (auto t = ((CoroExecutorImpl*)this)->tls(); t) {
                return (ContImpl*)*t;
            }

            return nullptr;
        }

        void yield() noexcept override;

        u32 random() noexcept override {
            return pool_->random().nextU32();
        }

        void parkWith(Runable* afterSuspend) noexcept {
            currentCont()->parkWith(afterSuspend);
        }

        CondVarIface* createCondVar() override;
        ChannelIface* createChannel(size_t cap) override;
        ThreadIface* createThread(Runable& runable) override;
        SemaphoreIface* createSemaphore(size_t initial) override;

        u32 poll(int fd, u32 flags, u64 deadlineUs) override;
    };

    struct PollRequestImpl: public PollRequest {
        ContImpl* cont;
        u32 result = 0;

        void parkWith(Runable&& afterSuspend) noexcept override {
            cont->parkWith(&afterSuspend);
        }

        void complete(u32 res) noexcept override {
            result = res;
            cont->reSchedule();
        }
    };
}

CoroExecutorImpl::CoroExecutorImpl(size_t threads, size_t reactors)
    : opool_(ObjPool::fromMemory())
    , tlsKey_(ThreadPool::registerTlsKey())
    , done_(this)
    , pool_(ThreadPool::workStealing(opool_.mutPtr(), threads + reactors))
{
    createPipeFD(joinR_, joinW_);
    createPipeFD(submitR_, submitW_);

    joinW_.setNonBlocking();
    submitR_.setNonBlocking();

    for (size_t i = 0; i < reactors; ++i) {
        reactors_.pushBack(ReactorIface::create(this, pool_, opool_.mutPtr()));
    }

    spawnRun(SpawnParams().setSystem(true).setRunable([this] {
        spawnSystem();
    }));
}

Cont* CoroExecutorImpl::spawnRun(SpawnParams params) {
    auto task = makeContImpl(this, params);

    if (tls()) {
        task->reSchedule();
    } else if (params.system) {
        task->reSchedule();
    } else {
        submitExternalTask(task);
    }

    return task;
}

void CoroExecutorImpl::yield() noexcept {
    struct Y: Runable {
        ContImpl* c;

        void run() noexcept override {
            c->reSchedule();
        }
    } y;

    (y.c = currentCont())->parkWith(&y);
}

void CoroExecutorImpl::spawnSystem() noexcept {
    for (auto r : reactors_) {
        done_.inc();

        spawnRun(
            SpawnParams()
                .setStack(opool_.mutPtr(), 16 * 1024)
                .setPriority(2)
                .setSystem(true)
                .setRunable([this, r] {
                    r->run();
                    done_.done();
                }));
    }

    done_.inc();

    spawnRun(
        SpawnParams()
            .setStack(opool_.mutPtr(), 16 * 1024)
            .setPriority(1)
            .setSystem(true)
            .setRunable([this] {
                submitterLoop();

                for (auto* r : reactors_) {
                    r->stop();
                }

                done_.done();
            }));
}

CoroExecutorImpl::~CoroExecutorImpl() noexcept {
    join();

    spawn([&] {
        done_.wait();
    });

    submitExternalTask(nullptr);

    join();
}

void CoroExecutorImpl::join() noexcept {
    while (stdAtomicFetch(&inflight_, MemoryOrder::Acquire) > 0) {
        char b;

        joinR_.read(&b, 1);
    }
}

void CoroExecutorImpl::submitExternalTask(Task* task) noexcept {
    submitW_.write(&task, sizeof(task));
}

void CoroExecutorImpl::submitterLoop() noexcept {
    Vector<Task*> buf(64);

    for (;;) {
        CoroExecutor::poll(submitR_.get(), PollFlag::In);

        auto n = ::read(submitR_.get(), buf.mutData(), (u8*)buf.mutStorageEnd() - (u8*)buf.mutData());

        STD_INSIST(n > 0);
        STD_INSIST(n % sizeof(Task*) == 0);

        auto cnt = (size_t)n / sizeof(Task*);

        for (auto task : range(buf.data(), buf.data() + cnt)) {
            if (task) {
                pool_->submitTask(task);
            } else {
                return;
            }
        }

        buf.grow(cnt + 1);
    }
}

ContImpl::ContImpl(CoroExecutorImpl* exec, void* ctxBuf, SpawnParams params) noexcept
    : exec_(exec)
    , ctx_{Context::create(ctxBuf, params.stackPtr, params.stackSize, *this)}
    , workerCtx_(nullptr)
    , runable_(params.runable)
    , priority_(params.priority)
{
}

UserContImpl::UserContImpl(CoroExecutorImpl* exec, void* ctxBuf, SpawnParams params) noexcept
    : ContImpl(exec, ctxBuf, params)
{
    stdAtomicAddAndFetch(&exec_->inflight_, 1, MemoryOrder::Relaxed);
}

UserContImpl::~UserContImpl() {
    if (stdAtomicAddAndFetch(&exec_->inflight_, -1, MemoryOrder::Release) == 0) {
        char b = 1;

        exec_->joinW_.write(&b, 1);
    }
}

u8 ContImpl::priority() const noexcept {
    return priority_;
}

void ContImpl::reSchedule() noexcept {
    exec_->pool_->submitTask(this);
}

void ContImpl::parkWith(Runable* afterSuspend) noexcept {
    runable_ = afterSuspend;
    STD_ASSERT(exec_->currentCont() == this);
    ctx_->switchTo(*workerCtx_);
}

void ContImpl::run() noexcept {
    if (workerCtx_) {
        exchange(runable_, nullptr)->run();
        ctx_->switchTo(*workerCtx_);

        return;
    }

    auto tls = exec_->tls();

    *tls = this;
    (workerCtx_ = Context::create(alloca(Context::implSize())))->switchTo(*ctx_.ptr);
    delete workerCtx_;
    workerCtx_ = nullptr;
    *tls = nullptr;

    if (auto* as = exchange(runable_, nullptr); as) {
        return as->run();
    }

    delete this;
}

SpawnParams::SpawnParams() noexcept
    : stackSize(16 * 1024)
    , stackPtr(nullptr)
    , runable(nullptr)
    , priority(0)
    , system(false)
{
}

u32 CoroExecutorImpl::poll(int fd, u32 flags, u64 deadlineUs) {
    PollRequestImpl req;

    req.cont = currentCont();
    req.fd = fd;
    req.flags = flags;
    req.deadline = deadlineUs;

    reactors_[splitMix64(fd) % reactors_.length()]->processRequest(&req);

    return req.result;
}

u64 Cont::id() const noexcept {
    return (u64)(size_t)this;
}

CondVarIface* CoroExecutorImpl::createCondVar() {
    struct CoroCondVarImpl: public CondVarIface {
        struct ParkCtx: public Runable {
            CoroCondVarImpl* cv;
            Mutex* mutex;

            ParkCtx(CoroCondVarImpl* cv, Mutex* mutex) noexcept
                : cv(cv)
                , mutex(mutex)
            {
            }

            void run() override {
                cv->queueMutex_.unlock();
                mutex->unlock();
            }
        };

        Mutex queueMutex_;
        IntrusiveList waiters_;

        CoroCondVarImpl(CoroExecutorImpl* exec) noexcept
            : queueMutex_(exec)
        {
        }

        CoroExecutorImpl* exec() noexcept {
            return (CoroExecutorImpl*)queueMutex_.nativeHandle();
        }

        void wait(Mutex& mutex) noexcept override {
            queueMutex_.lock();
            auto* cont = exec()->currentCont();
            waiters_.pushBack(cont);
            ParkCtx ctx(this, &mutex);
            cont->parkWith(&ctx);
            mutex.lock();
        }

        void signal() noexcept override {
            LockGuard guard(queueMutex_);

            if (auto* node = (ContImpl*)(Task*)waiters_.popFrontOrNull(); node) {
                node->reSchedule();
            }
        }

        void broadcast() noexcept override {
            LockGuard guard(queueMutex_);

            while (auto* node = (ContImpl*)(Task*)waiters_.popFrontOrNull()) {
                node->reSchedule();
            }
        }
    };

    return new CoroCondVarImpl(this);
}

ThreadIface* CoroExecutorImpl::createThread(Runable& runable) {
    struct State: public ARC {
        CoroExecutorImpl* exec_;
        Runable* runable_;
        Semaphore sem_;

        State(CoroExecutorImpl* exec, Runable& runable)
            : exec_(exec)
            , runable_(&runable)
            , sem_(0, exec)
        {
        }

        void run() {
            runable_->run();
            sem_.post();
        }

        auto start() {
            return exec_->spawn([ref = makeIntrusivePtr(this)] mutable {
                ref->run();
            });
        }

        void join() noexcept {
            sem_.wait();
        }
    };

    struct CoroThreadImpl: public ThreadIface {
        IntrusivePtr<State> state_;
        Cont* cont_ = nullptr;

        CoroThreadImpl(State* s) noexcept
            : state_(s)
        {
        }

        void start() override {
            cont_ = state_->start();
        }

        void join() noexcept override {
            state_->join();
        }

        void detach() noexcept override {
        }

        u64 threadId() const noexcept {
            return cont_->id();
        }
    };

    return new CoroThreadImpl(new State(this, runable));
}

ChannelIface* CoroExecutorImpl::createChannel(size_t cap) {
    struct CoroChannelImpl: public ChannelIface, public Runable {
        struct Waiter: public IntrusiveNode {
            ContImpl* cont;
            void* value;
            bool valueSet;
        };

        Mutex queueMutex_;
        IntrusiveList senders_;
        IntrusiveList receivers_;
        bool closed_;

        CoroChannelImpl(CoroExecutorImpl* exec) noexcept
            : queueMutex_(exec)
            , closed_(false)
        {
        }

        CoroExecutorImpl* exec() noexcept {
            return (CoroExecutorImpl*)queueMutex_.nativeHandle();
        }

        void run() override {
            queueMutex_.unlock();
        }

        bool sendOne(void* v) noexcept {
            if (auto* w = (Waiter*)receivers_.popFrontOrNull(); w) {
                w->value = v;
                w->valueSet = true;
                w->cont->reSchedule();

                return true;
            }

            return bufferOne(v);
        }

        bool recvOne(void** out) noexcept {
            if (unbufferOne(out)) {
                return true;
            }

            if (auto* w = (Waiter*)senders_.popFrontOrNull(); w) {
                *out = w->value;
                w->cont->reSchedule();

                return true;
            }

            return false;
        }

        virtual bool bufferOne(void*) noexcept {
            return false;
        }

        virtual bool unbufferOne(void**) noexcept {
            return false;
        }

        void enqueue(void* v) noexcept override {
            LockGuard guard(queueMutex_);

            STD_INSIST(!closed_);

            if (sendOne(v)) {
                return;
            }

            Waiter w;

            w.cont = exec()->currentCont();
            w.value = v;
            w.valueSet = true;

            senders_.pushBack(&w);
            guard.drop();
            w.cont->parkWith(this);
        }

        bool dequeue(void** out) noexcept override {
            LockGuard guard(queueMutex_);

            if (recvOne(out)) {
                return true;
            }

            if (closed_) {
                return false;
            }

            Waiter w;

            w.cont = exec()->currentCont();
            w.value = nullptr;
            w.valueSet = false;

            receivers_.pushBack(&w);
            guard.drop();
            w.cont->parkWith(this);

            if (w.valueSet) {
                *out = w.value;

                return true;
            }

            return false;
        }

        bool tryEnqueue(void* v) noexcept override {
            LockGuard guard(queueMutex_);

            STD_INSIST(!closed_);

            return sendOne(v);
        }

        bool tryDequeue(void** out) noexcept override {
            LockGuard guard(queueMutex_);

            return recvOne(out);
        }

        void close() noexcept override {
            LockGuard guard(queueMutex_);

            STD_INSIST(!closed_);
            STD_INSIST(senders_.empty());

            closed_ = true;

            while (auto w = (Waiter*)receivers_.popFrontOrNull()) {
                w->cont->reSchedule();
            }
        }
    };

    // buffered channel (cap > 0)
    struct CoroChannelImplN: public CoroChannelImpl {
        RingBuffer buf_;

        CoroChannelImplN(CoroExecutorImpl* exec, size_t capacity) noexcept
            : CoroChannelImpl(exec)
            , buf_((void**)(this + 1), capacity)
        {
        }

        void* operator new(size_t, void* p) noexcept {
            return p;
        }

        void operator delete(void* p) noexcept {
            freeMemory(p);
        }

        bool bufferOne(void* v) noexcept override {
            if (!buf_.full()) {
                buf_.push(v);

                return true;
            }

            return false;
        }

        bool unbufferOne(void** out) noexcept override {
            if (!buf_.empty()) {
                *out = buf_.pop();

                if (auto* w = (Waiter*)senders_.popFrontOrNull(); w) {
                    buf_.push(w->value);
                    w->cont->reSchedule();
                }

                return true;
            }

            return false;
        }
    };

    if (cap == 0) {
        return new CoroChannelImpl(this);
    }

    return new (allocateMemory(sizeof(CoroChannelImplN) + cap * sizeof(void*))) CoroChannelImplN(this, cap);
}

SemaphoreIface* CoroExecutorImpl::createSemaphore(size_t initial) {
    struct CoroSemaphoreImpl: public SemaphoreIface, public Runable {
        CoroExecutorImpl* exec_;
        Mutex lock_;
        IntrusiveList waiters_;
        size_t count_;

        CoroSemaphoreImpl(CoroExecutorImpl* exec, size_t initial) noexcept
            : exec_(exec)
            , lock_(Mutex::spinLock(exec))
            , count_(initial)
        {
        }

        void run() override {
            lock_.unlock();
        }

        void post() noexcept override {
            lock_.lock();

            if (auto* cont = (ContImpl*)(Task*)waiters_.popFrontOrNull(); cont) {
                lock_.unlock();
                cont->reSchedule();
            } else {
                ++count_;
                lock_.unlock();
            }
        }

        void wait() noexcept override {
            lock_.lock();

            if (count_ > 0) {
                --count_;
                lock_.unlock();
                return;
            }

            auto* cont = exec_->currentCont();
            waiters_.pushBack(cont);
            cont->parkWith(this);
        }

        void* nativeHandle() noexcept override {
            return exec_;
        }

        bool tryWait() noexcept override {
            LockGuard guard(lock_);

            if (count_ > 0) {
                --count_;

                return true;
            }

            return false;
        }
    };

    return new CoroSemaphoreImpl(this, initial);
}

CoroExecutor::~CoroExecutor() noexcept {
}

CoroExecutor::Ref CoroExecutor::create(size_t threads) {
    return create(threads, max(threads / 4, (size_t)1));
}

CoroExecutor::Ref CoroExecutor::create(size_t threads, size_t reactors) {
    return new CoroExecutorImpl(threads, reactors);
}

u64 CoroExecutor::currentCoroId() const noexcept {
    return me()->id();
}

void CoroExecutor::sleep(u64 deadlineUs) {
    poll(-1, 0, deadlineUs);
}

void CoroExecutor::sleep() {
    sleep(UINT64_MAX);
}

void CoroExecutor::sleepTout(u64 timeoutUs) {
    sleep(monotonicNowUs() + timeoutUs);
}

u32 CoroExecutor::poll(int fd, u32 flags) {
    for (;;) {
        if (auto res = poll(fd, flags, UINT64_MAX); res) {
            return res;
        }
    }
}

SpawnParams& SpawnParams::setStackSize(size_t v) noexcept {
    stackSize = v;

    return *this;
}

SpawnParams& SpawnParams::setStackPtr(void* v) noexcept {
    stackPtr = v;

    return *this;
}

SpawnParams& SpawnParams::setPriority(u8 v) noexcept {
    priority = v;

    return *this;
}

SpawnParams& SpawnParams::setSystem(bool v) noexcept {
    system = v;

    return *this;
}

SpawnParams& SpawnParams::setRunablePtr(Runable* v) noexcept {
    runable = v;

    return *this;
}

SpawnParams& SpawnParams::setStack(void* v, size_t len) noexcept {
    return setStackPtr(v).setStackSize(len);
}

SpawnParams& SpawnParams::setStack(ObjPool* v, size_t len) noexcept {
    return setStack(v->allocate(len), len);
}
