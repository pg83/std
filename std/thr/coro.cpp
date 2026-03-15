#include "coro.h"
#include "task.h"
#include "pool.h"
#include "mutex.h"
#include "thread.h"
#include "context.h"
#include "reactor.h"
#include "cond_var.h"
#include "mutex_iface.h"
#include "thread_iface.h"
#include "channel_iface.h"
#include "cond_var_iface.h"

#include <std/sys/fd.h>
#include <std/sys/crt.h>
#include <std/rng/pcg.h>
#include <std/lib/list.h>
#include <std/sym/i_map.h>
#include <std/sys/atomic.h>
#include <std/lib/vector.h>
#include <std/dbg/insist.h>
#include <std/ptr/scoped.h>
#include <std/alg/minmax.h>
#include <std/alg/exchange.h>
#include <std/alg/destruct.h>
#include <std/lib/ring_buf.h>
#include <std/mem/obj_pool.h>

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
        Runable* afterSuspend_;
        u8 priority_;
        bool system_;

        u8 priority() const noexcept override;

        ContImpl(CoroExecutorImpl* exec, void* ctxBuf, SpawnParams params) noexcept;

        virtual ~ContImpl();

        void operator delete(void* p) noexcept {
            freeMemory(p);
        }

        void parkWith(Runable* afterSuspend) noexcept;
        void run() noexcept override;
        void reSchedule() noexcept;
    };

    struct alignas(max_align_t) HeapContImpl: public ContImpl {
        void* operator new(size_t, size_t stackSize) {
            return allocateMemory(sizeof(HeapContImpl) + Context::implSize() + stackSize);
        }

        HeapContImpl(CoroExecutorImpl* exec, SpawnParams params) noexcept
            : ContImpl(exec, (char*)(this + 1), params.setStackPtr((char*)(this + 1) + Context::implSize()))
        {
        }
    };

    struct alignas(max_align_t) ExtStackContImpl: public ContImpl {
        void* operator new(size_t sz) {
            return allocateMemory(sz + Context::implSize());
        }

        ExtStackContImpl(CoroExecutorImpl* exec, SpawnParams params) noexcept
            : ContImpl(exec, (char*)(this + 1), params)
        {
        }
    };

    ContImpl* makeContImpl(CoroExecutorImpl* exec, SpawnParams params) {
        if (params.stackPtr) {
            return new ExtStackContImpl(exec, params);
        } else {
            return new (params.stackSize) HeapContImpl(exec, params);
        }
    }

    struct CoroMutexImpl;
    struct CoroCondVarImpl;
    struct CoroChannelImpl;
    struct CoroChannelImplN;

    struct CoroExecutorImpl: public CoroExecutor {
        alignas(64) int inflight_ = 0;
        ObjPool::Ref opool_;
        const u64 tlsKey_;
        Vector<ReactorIface*> reactors_;
        ScopedFD joinR_;
        ScopedFD joinW_;
        ThreadPool* pool_;

        CoroExecutorImpl(size_t threads, size_t reactors);
        ~CoroExecutorImpl() noexcept override;

        void join() noexcept override;

        auto tls() {
            return pool_->tls(tlsKey_);
        }

        ContImpl* currentCont() {
            return (ContImpl*)*tls();
        }

        Cont* spawnRun(SpawnParams params) override {
            auto res = makeContImpl(this, params);

            pool_->submitTask(res);

            return res;
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

        ReactorIface* pickReactor() noexcept;

        MutexIface* createMutex() override;
        CondVarIface* createCondVar() override;
        ChannelIface* createChannel(size_t cap) override;
        ThreadIface* createThread(Runable& runable) override;

        u32 poll(int fd, u32 flags, u64 deadlineUs) override;
    };

    struct CoroThreadImpl: public ThreadIface {
        CoroExecutorImpl* exec_;
        Cont* cont_ = nullptr;
        Runable* runable_;
        Mutex mtx_;
        CondVar cv_;

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
        void close() override;
        void enqueue(void* v) override;
        bool dequeue(void** out) override;
        bool tryEnqueue(void* v) override;
        bool tryDequeue(void** out) override;
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
    , pool_(ThreadPool::workStealing(opool_.mutPtr(), threads + reactors))
{
    createPipeFD(joinR_, joinW_);

    for (size_t i = 0; i < reactors; ++i) {
        reactors_.pushBack(ReactorIface::create(this, pool_, opool_.mutPtr()));
    }

    for (auto r : reactors_) {
        spawnRun(SpawnParams().setStack(opool_.mutPtr(), 16 * 1024).setPriority(1).setSystem(true).setRunablePtr(r));
    }
}

CoroExecutorImpl::~CoroExecutorImpl() noexcept {
    join();

    for (auto* r : reactors_) {
        r->join();
    }
}

void CoroExecutorImpl::join() noexcept {
    while (stdAtomicFetch(&inflight_, MemoryOrder::Acquire) > 0) {
        char b;

        joinR_.read(&b, 1);
    }
}

ReactorIface* CoroExecutorImpl::pickReactor() noexcept {
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
        ((ContImpl*)(Task*)node)->reSchedule();
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

ContImpl::ContImpl(CoroExecutorImpl* exec, void* ctxBuf, SpawnParams params) noexcept
    : exec_(exec)
    , ctx_{Context::create(ctxBuf, params.stackPtr, params.stackSize, *this)}
    , workerCtx_(nullptr)
    , runable_(params.runable)
    , afterSuspend_(nullptr)
    , priority_(params.priority)
    , system_(params.system)
{
    if (!system_) {
        stdAtomicAddAndFetch(&exec_->inflight_, 1, MemoryOrder::Relaxed);
    }
}

u8 ContImpl::priority() const noexcept {
    return priority_;
}

void ContImpl::reSchedule() noexcept {
    exec_->pool_->submitTask(this);
}

void ContImpl::parkWith(Runable* afterSuspend) noexcept {
    afterSuspend_ = afterSuspend;
    ctx_->switchTo(*workerCtx_);
}

void ContImpl::run() noexcept {
    if (workerCtx_) {
        runable_->run();
        runable_ = nullptr;
        ctx_->switchTo(*workerCtx_);

        return;
    }

    *exec_->tls() = this;
    (workerCtx_ = Context::create(alloca(Context::implSize())))->switchTo(*ctx_.ptr);
    delete workerCtx_;
    workerCtx_ = nullptr;

    if (auto* as = exchange(afterSuspend_, nullptr); as) {
        as->run();
    } else if (runable_) {
        reSchedule();
    } else {
        delete this;
    }
}

ContImpl::~ContImpl() {
    if (!system_) {
        if (stdAtomicAddAndFetch(&exec_->inflight_, -1, MemoryOrder::Release) == 0) {
            char b = 1;

            exec_->joinW_.write(&b, 1);
        }
    }
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

    pickReactor()->processRequest(&req);

    return req.result;
}

u64 Cont::id() const noexcept {
    return (u64)(size_t)this;
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
        ((ContImpl*)(Task*)node)->reSchedule();
    }
}

void CoroCondVarImpl::broadcast() noexcept {
    LockGuard guard(queueMutex_);

    while (auto* node = waiters_.popFrontOrNull()) {
        ((ContImpl*)(Task*)node)->reSchedule();
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
    cont_ = exec_->spawnRun(SpawnParams().setRunable([this]() {
        runable_->run();
        notifyDone();
    }));
}

void CoroThreadImpl::notifyDone() noexcept {
    LockGuard guard(mtx_);

    runable_ = nullptr;
    cv_.signal();
}

void CoroThreadImpl::join() noexcept {
    LockGuard(mtx_).run([this]() {
        while (runable_) {
            cv_.wait(mtx_);
        }
    });

    delete this;
}

void CoroThreadImpl::detach() noexcept {
    exec_->spawn([this]() {
        join();
    });
}

u64 CoroThreadImpl::threadId() const noexcept {
    return cont_->id();
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

    while (auto w = (Waiter*)receivers_.popFrontOrNull()) {
        w->cont->reSchedule();
    }
}

// CoroChannelImplN — buffered channel (cap > 0)

CoroChannelImplN::CoroChannelImplN(CoroExecutorImpl* exec, size_t capacity) noexcept
    : CoroChannelImpl(exec)
    , buf_((void**)(this + 1), capacity)
{
}

bool CoroChannelImpl::sendOne(void* v) noexcept {
    if (auto* w = (Waiter*)receivers_.popFrontOrNull(); w) {
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

    if (auto* w = (Waiter*)senders_.popFrontOrNull(); w) {
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

        if (auto* w = (Waiter*)senders_.popFrontOrNull(); w) {
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

CoroExecutor::Ref CoroExecutor::create(size_t threads) {
    return create(threads, max(threads / 4, (size_t)1));
}

CoroExecutor::Ref CoroExecutor::create(size_t threads, size_t reactors) {
    return new CoroExecutorImpl(threads, reactors);
}

u64 CoroExecutor::currentCoroId() const noexcept {
    return me()->id();
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
