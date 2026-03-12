#include "pool.h"
#include "task.h"
#include "mutex.h"
#include "thread.h"
#include "runable.h"
#include "cond_var.h"
#include "wait_queue.h"

#include <std/rng/pcg.h>
#include <std/lib/list.h>
#include <std/sym/i_map.h>
#include <std/alg/range.h>
#include <std/alg/minmax.h>
#include <std/lib/vector.h>
#include <std/dbg/assert.h>
#include <std/dbg/insist.h>
#include <std/sys/atomic.h>
#include <std/alg/shuffle.h>
#include <std/alg/exchange.h>

#include <stdlib.h>
#include <sched.h>

using namespace stl;

u64 stl::registerTlsKey() noexcept {
    static u64 g_tlsKeyCounter = 0;

    return stdAtomicAddAndFetch(&g_tlsKeyCounter, 1, MemoryOrder::Relaxed);
}

namespace {
    struct ShutDown: public Task {
        ~ShutDown() noexcept {
            unlink();
        }

        void run() override {
            throw this;
        }
    };

    struct SyncThreadPool: public ThreadPool {
        IntMap<void*> tls_;
        PCG32 rng_{this};

        void submitTask(Task* task) noexcept override {
            task->run();
        }

        void join() noexcept override {
        }

        void** tls(u64 key) noexcept override {
            return &tls_[key];
        }

        PCG32& random() noexcept override {
            return rng_;
        }

        size_t numThreads() const noexcept override {
            return 0;
        }
    };

    class ThreadPoolImpl: public ThreadPool {
        struct Worker: public Runable {
            ThreadPoolImpl* pool_;
            IntMap<void*> tls_;
            PCG32 rng_;
            Thread thread_;

            explicit Worker(ThreadPoolImpl* p) noexcept
                : pool_(p)
                , rng_(this)
                , thread_(*this)
            {
            }

            auto key() const noexcept {
                return thread_.threadId();
            }

            void run() noexcept override {
                try {
                    pool_->workerLoop();
                } catch (ShutDown* sh) {
                    pool_->submitTask(sh);
                }
            }
        };

        Mutex mutex_;
        CondVar condVar_;
        IntrusiveList queue_;
        IntMap<Worker> workerIndex_;
        size_t inflight_ = 0;

        void workerLoop();

    public:
        ThreadPoolImpl(size_t numThreads);
        ~ThreadPoolImpl() noexcept;

        void submitTask(Task* task) noexcept override;
        void join() noexcept override;
        void** tls(u64 key) noexcept override;
        PCG32& random() noexcept override;

        size_t numThreads() const noexcept override {
            return workerIndex_.size();
        }
    };
}

ThreadPoolImpl::ThreadPoolImpl(size_t numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        workerIndex_.insertKeyed(this);
    }
}

void ThreadPoolImpl::submitTask(Task* task) noexcept {
    LockGuard lock(mutex_);
    ++inflight_;
    queue_.pushBack(task);
    condVar_.signal();
}

void ThreadPoolImpl::join() noexcept {
    LockGuard lock(mutex_);

    while (inflight_) {
        UnlockGuard unlock(mutex_);

        sched_yield();
    }
}

ThreadPoolImpl::~ThreadPoolImpl() noexcept {
    join();

    ShutDown task;

    submitTask(&task);

    workerIndex_.visit([](Worker& w) {
        w.thread_.join();
    });
}

void** ThreadPoolImpl::tls(u64 key) noexcept {
    if (auto w = workerIndex_.find(Thread::currentThreadId()); w) {
        return &w->tls_[key];
    }

    return nullptr;
}

PCG32& ThreadPoolImpl::random() noexcept {
    return workerIndex_.find(Thread::currentThreadId())->rng_;
}

void ThreadPoolImpl::workerLoop() {
    LockGuard lock(mutex_);

    for (;; condVar_.wait(mutex_)) {
        while (auto t = (Task*)queue_.popFrontOrNull()) {
            {
                UnlockGuard unlock(mutex_);

                t->run();
            }

            --inflight_;
        }
    }
}

u8 Task::priority() const noexcept {
    return 0;
}

ThreadPool::~ThreadPool() noexcept {
}

ThreadPool::Ref ThreadPool::sync() {
    return new SyncThreadPool();
}

ThreadPool::Ref ThreadPool::simple(size_t threads) {
    if (threads == 0) {
        return sync();
    }

    return new ThreadPoolImpl(threads);
}

namespace {
    struct WorkStealingThreadPool: public ThreadPool {
        struct LocalWorker {
            IntMap<void*> tls_;
            PCG32 rng_;

            explicit LocalWorker(u64 seed) noexcept
                : rng_(seed)
            {
            }

            virtual void steal(IntrusiveList* stolen) noexcept = 0;
            virtual void pushThrLocal(Task* task) noexcept = 0;

            void** tls(u64 key) noexcept {
                return &tls_[key];
            }

            PCG32& random() noexcept {
                return rng_;
            }
        };

        struct Worker: public LocalWorker, public Runable, public WaitQueue::Item {
            WorkStealingThreadPool* pool_;
            Vector<LocalWorker*> so_;
            Mutex mutex_;
            CondVar condVar_;
            IntrusiveList tasks_;
            IntrusiveList local_;
            Thread thread_;

            Worker(WorkStealingThreadPool* pool, u32 myIndex, u64 seed);

            auto key() const noexcept {
                return thread_.threadId();
            }

            void flushLocal() noexcept {
                tasks_.pushFront(local_);
            }

            Task* popNoLock() noexcept {
                return (Task*)tasks_.popFrontOrNull();
            }

            template <typename T>
            void push(T& task) noexcept;

            void pushThrLocal(Task* task) noexcept override {
                if (task->priority()) {
                    local_.pushFront(task);
                } else {
                    local_.pushBack(task);
                }
            }

            void notify() noexcept {
                LockGuard lock(mutex_);
                condVar_.signal();
            }

            void join() noexcept;

            void sleep() noexcept {
                pool_->wq->enqueue(this);
                condVar_.wait(mutex_);
            }

            void loop();
            void run() noexcept override;
            void initStealOrder() noexcept;
            void trySteal(IntrusiveList* stolen) noexcept;
            void steal(IntrusiveList* stolen) noexcept override;
        };

        IntMap<Worker> workerIndex_;
        WaitQueue::Ref wq;

        WorkStealingThreadPool(size_t numThreads);
        ~WorkStealingThreadPool() noexcept;

        bool notifyOne() noexcept;
        void join() noexcept override;
        PCG32& random() noexcept override;
        Worker* localWorker() noexcept;
        void** tls(u64 key) noexcept override;
        void submitTask(Task* task) noexcept override;

        size_t numThreads() const noexcept override {
            return workerIndex_.size();
        }
    };
}

template <typename T>
void WorkStealingThreadPool::Worker::push(T& task) noexcept {
    LockGuard lock(mutex_);
    flushLocal();
    tasks_.pushBack(task);
    condVar_.signal();
}

WorkStealingThreadPool::WorkStealingThreadPool(size_t numThreads)
    : wq(WaitQueue::construct(numThreads))
{
    PCG32 rng{(size_t)this};

    for (size_t i = 0; i < numThreads; ++i) {
        workerIndex_.insertKeyed(this, (u32)i, rng.nextU64());
    }

    workerIndex_.visit([](Worker& w) {
        w.initStealOrder();
        w.mutex_.unlock();
    });
}

bool WorkStealingThreadPool::notifyOne() noexcept {
    if (auto item = (Worker*)wq->dequeue()) {
        item->notify();

        return true;
    }

    return false;
}

WorkStealingThreadPool::Worker* WorkStealingThreadPool::localWorker() noexcept {
    static thread_local Worker* curw = nullptr;

    if (curw) {
        return curw;
    } else if (auto w = workerIndex_.find(Thread::currentThreadId()); w) {
        return curw = w;
    }

    return nullptr;
}

void WorkStealingThreadPool::submitTask(Task* task) noexcept {
    if (auto w = localWorker(); w) {
        return w->pushThrLocal(task);
    }

    while (true) {
        if (auto w = (Worker*)wq->dequeue()) {
            return w->push(task);
        }

        sched_yield();
    }
}

void** WorkStealingThreadPool::tls(u64 key) noexcept {
    if (auto w = localWorker()) {
        return w->tls(key);
    }

    return nullptr;
}

PCG32& WorkStealingThreadPool::random() noexcept {
    return localWorker()->random();
}

void WorkStealingThreadPool::join() noexcept {
    while (wq->sleeping() != workerIndex_.size()) {
        sched_yield();
    }
}

WorkStealingThreadPool::~WorkStealingThreadPool() noexcept {
    join();

    workerIndex_.visit([](Worker& w) {
        w.join();
    });
}

WorkStealingThreadPool::Worker::Worker(WorkStealingThreadPool* pool, u32 myIndex, u64 seed)
    : LocalWorker(seed)
    , WaitQueue::Item{nullptr, (u8)myIndex}
    , pool_(pool)
    , mutex_(true)
    , thread_(*this)
{
}

void WorkStealingThreadPool::Worker::join() noexcept {
    ShutDown sh;

    {
        LockGuard lock(mutex_);

        pushThrLocal(&sh);
    }

    notify();
    thread_.join();
}

void WorkStealingThreadPool::Worker::initStealOrder() noexcept {
    pool_->workerIndex_.visit([this](Worker& w) {
        if (&w != this) {
            so_.pushBack(&w);
        }
    });

    shuffle(rng_, so_.mutBegin(), so_.mutEnd());
}

void WorkStealingThreadPool::Worker::trySteal(IntrusiveList* stolen) noexcept {
    const u32 n = (u32)so_.length();
    const u32 offset = rng_.uniformUnbiased(n);

    for (u32 i = 0; stolen->empty() && i < n; ++i) {
        so_[(offset + i) % n]->steal(stolen);
    }
}

void WorkStealingThreadPool::Worker::run() noexcept {
    try {
        loop();
    } catch (ShutDown* sh) {
    }
}

void WorkStealingThreadPool::Worker::loop() {
    LockGuard lock(mutex_);

    do {
        while (auto task = (Task*)local_.popFrontOrNull()) {
            task->run();
        }

        while (auto task = popNoLock()) {
            if (!tasks_.empty()) {
                pool_->notifyOne();
            }

            {
                UnlockGuard unlock(mutex_);

                task->run();
            }

            flushLocal();
        }

        IntrusiveList stolen;

        {
            UnlockGuard unlock(mutex_);

            trySteal(&stolen);
        }

        local_.pushBack(stolen);

        flushLocal();
    } while (!tasks_.empty() || (sleep(), true));
}

void WorkStealingThreadPool::Worker::steal(IntrusiveList* stolen) noexcept {
    LockGuard lock(mutex_);

    tasks_.splitHalf(tasks_, *stolen);

    if (stolen->empty()) {
        tasks_.xchgWithEmptyList(*stolen);
    }
}

ThreadPool::Ref ThreadPool::workStealing(size_t threads) {
    if (threads <= 1) {
        return simple(threads);
    }

    return new WorkStealingThreadPool(threads);
}
