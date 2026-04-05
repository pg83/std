#include "coro.h"
#include "task.h"
#include "pool.h"
#include "mutex.h"
#include "guard.h"
#include "thread.h"
#include "poll_fd.h"
#include "context.h"
#include "cond_var.h"
#include "semaphore.h"
#include "event_iface.h"
#include "thread_iface.h"
#include "reactor_poll.h"
#include "cond_var_iface.h"
#include "semaphore_iface.h"

#include <std/sys/fd.h>
#include <std/sys/crt.h>
#include <std/mem/new.h>
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
#include <std/mem/obj_pool.h>
#include <std/net/dns_iface.h>
#include <std/rng/split_mix_64.h>

#include <errno.h>
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

        ContImpl(CoroExecutorImpl* exec, void* ctxBuf, SpawnParams params) noexcept;
        virtual ~ContImpl();

        void operator delete(void* p) noexcept {
            freeMemory(p);
        }

        void parkWith(Runable* afterSuspend) noexcept;

        void parkWith(Runable&& afterSuspend) noexcept {
            parkWith(&afterSuspend);
        }

        template <typename F>
        void park(F f) noexcept {
            parkWith(makeRunable(f));
        }

        void run() noexcept override;
        void reSchedule() noexcept;
    };

    struct alignas(max_align_t) HeapContImpl final: public ContImpl {
        void* operator new(size_t, size_t stackSize) {
            return allocateMemory(sizeof(HeapContImpl) + Context::implSize() + stackSize);
        }

        HeapContImpl(CoroExecutorImpl* exec, SpawnParams params) noexcept
            : ContImpl(exec, (char*)(this + 1), params.setStackPtr((char*)(this + 1) + Context::implSize()))
        {
        }
    };

    struct alignas(max_align_t) ExtStackContImpl final: public ContImpl {
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

    struct JoinPipe {
        ScopedFD r;
        ScopedFD w;

        JoinPipe() {
            createPipeFD(r, w);
            w.setNonBlocking();
        }
    };

    struct CoroExecutorImpl: public CoroExecutor {
        alignas(64) int inflight_ = 0;
        JoinPipe* join_;
        const u64 tlsKey_;
        Vector<ReactorIface*> reactors_;
        Vector<DnsResolver*> dnsResolvers_;
        ThreadPool* offload_;
        ThreadPool* pool_;

        CoroExecutorImpl(ObjPool* pool, size_t threads, size_t reactors);
        ~CoroExecutorImpl() noexcept;

        void join() noexcept override;
        Cont* spawnRun(SpawnParams params) override;

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

        void createEvent(void* buf) override;
        ThreadIface* createThread() override;
        CondVarIface* createCondVar() override;
        SemaphoreIface* createSemaphore(size_t initial) override;

        DnsResult* resolve(ObjPool* pool, const StringView& name) override;

        int fsync(int fd) override;
        int fdatasync(int fd) override;
        ssize_t pread(int fd, void* buf, size_t len, off_t offset) override;
        ssize_t pwrite(int fd, const void* buf, size_t len, off_t offset) override;

        void parkWith(Runable&&, Task**) noexcept override;
        void offloadRun(ThreadPool* pool, Runable&& work) override;

        u32 poll(PollFD pfd, u64 deadlineUs) override;
        size_t poll(PollGroup* g, PollFD* out, u64 deadlineUs) override;
    };
}

CoroExecutorImpl::CoroExecutorImpl(ObjPool* pool, size_t threads, size_t reactors)
    : join_(pool->make<JoinPipe>())
    , tlsKey_(ThreadPool::registerTlsKey())
    , pool_(ThreadPool::workStealing(pool, threads))
{
    for (size_t i = 0; i < reactors; ++i) {
        reactors_.pushBack(ReactorIface::create(this, pool_, pool));
    }

    offload_ = ThreadPool::simple(pool, reactors);

    for (size_t i = 0; i < reactors; ++i) {
        dnsResolvers_.pushBack(DnsResolver::create(pool, this, offload_));
    }
}

Cont* CoroExecutorImpl::spawnRun(SpawnParams params) {
    auto task = makeContImpl(this, params);

    task->reSchedule();

    return task;
}

void CoroExecutorImpl::yield() noexcept {
    auto c = currentCont();

    c->park([c] {
        c->reSchedule();
    });
}

CoroExecutorImpl::~CoroExecutorImpl() noexcept {
    join();
}

void CoroExecutorImpl::join() noexcept {
    while (stdAtomicFetch(&inflight_, MemoryOrder::Acquire) > 0) {
        char b;

        join_->r.read(&b, 1);
    }
}

ContImpl::ContImpl(CoroExecutorImpl* exec, void* ctxBuf, SpawnParams params) noexcept
    : exec_(exec)
    , ctx_{Context::create(ctxBuf, params.stackPtr, params.stackSize, *this)}
    , workerCtx_(nullptr)
    , runable_(params.runable)
{
    stdAtomicAddAndFetch(&exec_->inflight_, 1, MemoryOrder::Relaxed);
}

ContImpl::~ContImpl() {
    if (stdAtomicAddAndFetch(&exec_->inflight_, -1, MemoryOrder::Release) == 0) {
        char b = 1;

        exec_->join_->w.write(&b, 1);
    }
}

void ContImpl::reSchedule() noexcept {
    STD_ASSERT(!workerCtx_);
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

    if (auto as = runable_; as) {
        runable_ = nullptr;
        return as->run();
    }

    delete this;
}

SpawnParams::SpawnParams() noexcept
    : stackSize(16 * 1024)
    , stackPtr(nullptr)
    , runable(nullptr)
{
}

void CoroExecutorImpl::parkWith(Runable&& afterSuspend, Task** out) noexcept {
    auto cont = currentCont();
    *out = cont;
    cont->parkWith(&afterSuspend);
}

DnsResult* CoroExecutorImpl::resolve(ObjPool* pool, const StringView& name) {
    return dnsResolvers_[name.hash64() % dnsResolvers_.length()]->resolve(pool, name);
}

u32 CoroExecutorImpl::poll(PollFD pfd, u64 deadlineUs) {
    return reactors_[splitMix64(pfd.fd) % reactors_.length()]->poll(pfd, deadlineUs);
}

size_t CoroExecutorImpl::poll(PollGroup* g, PollFD* out, u64 deadlineUs) {
    return reactors_[splitMix64(g->fd()) % reactors_.length()]->poll(g, out, deadlineUs);
}

void CoroExecutorImpl::offloadRun(ThreadPool* pool, Runable&& work) {
    auto cont = currentCont();

    cont->park([&] {
        pool->submit([&] {
            work.run();
            cont->reSchedule();
        });
    });
}

ssize_t CoroExecutorImpl::pread(int fd, void* buf, size_t len, off_t offset) {
    ssize_t result = 0;

    // clang-format off
    offload(offload_, [&] {
        ssize_t n = ::pread(fd, buf, len, offset);
        result = n < 0 ? -errno : n;
    });
    // clang-format on

    return result;
}

ssize_t CoroExecutorImpl::pwrite(int fd, const void* buf, size_t len, off_t offset) {
    ssize_t result = 0;

    // clang-format off
    offload(offload_, [&] {
        ssize_t n = ::pwrite(fd, buf, len, offset);
        result = n < 0 ? -errno : n;
    });
    // clang-format on

    return result;
}

int CoroExecutorImpl::fsync(int fd) {
    int result = 0;

    // clang-format off
    offload(offload_, [&] {
        result = ::fsync(fd) < 0 ? -errno : 0;
    });
    // clang-format on

    return result;
}

int CoroExecutorImpl::fdatasync(int fd) {
    int result = 0;

    // clang-format off
    offload(offload_, [&] {
#if defined(__APPLE__)
        result = ::fcntl(fd, F_FULLFSYNC) < 0 ? -errno : 0;
#else
        result = ::fdatasync(fd) < 0 ? -errno : 0;
#endif
    });
    // clang-format on

    return result;
}

u64 Cont::id() const noexcept {
    return (u64)(size_t)this;
}

void CoroExecutorImpl::createEvent(void* buf) {
    struct CoroEventImpl: public EventIface, public Newable {
        CoroExecutorImpl* exec_;
        ContImpl* waiter_;

        CoroEventImpl(CoroExecutorImpl* exec) noexcept
            : exec_(exec)
            , waiter_(nullptr)
        {
        }

        void wait(Runable& cb) noexcept override {
            (waiter_ = exec_->currentCont())->parkWith(&cb);
        }

        void signal() noexcept override {
            waiter_->reSchedule();
        }

        static void operator delete(void*) noexcept {
        }
    };

    new (buf) CoroEventImpl(this);
}

CondVarIface* CoroExecutorImpl::createCondVar() {
    struct CoroCondVarImpl: public CondVarIface {
        Mutex queueMutex_;
        IntrusiveList waiters_;

        CoroCondVarImpl(CoroExecutorImpl* exec) noexcept
            : queueMutex_(Mutex::spinLock(exec))
        {
        }

        CoroExecutorImpl* exec() noexcept {
            return (CoroExecutorImpl*)queueMutex_.nativeHandle();
        }

        void wait(Mutex& mutex) noexcept override {
            queueMutex_.lock();
            auto cont = exec()->currentCont();
            waiters_.pushBack(cont);
            cont->park([&] {
                queueMutex_.unlock();
                mutex.unlock();
            });
            mutex.lock();
        }

        void signal() noexcept override {
            auto node = LockGuard(queueMutex_).run([&] {
                return (ContImpl*)(Task*)waiters_.popFrontOrNull();
            });

            if (node) {
                node->reSchedule();
            }
        }

        void broadcast() noexcept override {
            IntrusiveList tmp;

            LockGuard(queueMutex_).run([&] {
                tmp.xchg(waiters_);
            });

            while (auto node = (ContImpl*)(Task*)tmp.popFrontOrNull()) {
                node->reSchedule();
            }
        }
    };

    return new CoroCondVarImpl(this);
}

ThreadIface* CoroExecutorImpl::createThread() {
    struct State: public ARC {
        Semaphore sem_;

        State(CoroExecutorImpl* exec)
            : sem_(0, exec)
        {
        }

        CoroExecutorImpl* exec() noexcept {
            return (CoroExecutorImpl*)sem_.nativeHandle();
        }

        void run(Runable& runable) {
            runable.run();
            sem_.post();
        }

        auto start(Runable& runable) {
            return exec()->spawn([ref = makeIntrusivePtr(this), &runable] mutable {
                ref->run(runable);
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

        void start(Runable& runable) override {
            cont_ = state_->start(runable);
        }

        void join() noexcept override {
            state_->join();
        }

        void detach() noexcept override {
        }

        u64 threadId() const noexcept override {
            return cont_->id();
        }
    };

    return new CoroThreadImpl(new State(this));
}

SemaphoreIface* CoroExecutorImpl::createSemaphore(size_t initial) {
    struct CoroSemaphoreImpl: public SemaphoreIface {
        Mutex lock_;
        IntrusiveList waiters_;
        size_t count_;

        CoroSemaphoreImpl(CoroExecutorImpl* exec, size_t initial) noexcept
            : lock_(Mutex::spinLock(exec))
            , count_(initial)
        {
        }

        void post() noexcept override {
            lock_.lock();

            if (auto cont = (ContImpl*)(Task*)waiters_.popFrontOrNull(); cont) {
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

            auto cont = ((CoroExecutorImpl*)(CoroExecutor*)nativeHandle())->currentCont();
            waiters_.pushBack(cont);
            cont->park([&] {
                lock_.unlock();
            });
        }

        void* nativeHandle() noexcept override {
            return lock_.nativeHandle();
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

CoroExecutor* CoroExecutor::create(ObjPool* pool, size_t threads) {
    return create(pool, threads, threads);
}

CoroExecutor* CoroExecutor::create(ObjPool* pool, size_t threads, size_t reactors) {
    return pool->make<CoroExecutorImpl>(pool, threads, reactors);
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

u32 CoroExecutor::poll(int fd, u32 flags, u64 deadlineUs) {
    return poll({fd, flags}, deadlineUs);
}

u32 CoroExecutor::poll(int fd, u32 flags) {
    for (;;) {
        if (auto res = poll({fd, flags}, UINT64_MAX); res) {
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
