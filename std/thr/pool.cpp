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
#include <std/dbg/insist.h>
#include <std/sys/atomic.h>
#include <std/alg/shuffle.h>
#include <std/alg/exchange.h>
#include <std/rng/split_mix_64.h>

#include <stdlib.h>

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

        void submitTask(Task* task) noexcept override {
            task->run();
        }

        void join() noexcept override {
        }

        void** tls(u64 key) noexcept override {
            return &tls_[key];
        }
    };

    class ThreadPoolImpl: public ThreadPool {
        struct Worker: public Runable {
            ThreadPoolImpl* pool_;
            IntMap<void*> tls_;
            Thread thread_;

            explicit Worker(ThreadPoolImpl* p) noexcept
                : pool_(p)
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

        void workerLoop();

    public:
        ThreadPoolImpl(size_t numThreads);

        void submitTask(Task* task) noexcept override;
        void join() noexcept override;
        void** tls(u64 key) noexcept override;
    };
}

ThreadPoolImpl::ThreadPoolImpl(size_t numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        workerIndex_.insertKeyed(this);
    }
}

void ThreadPoolImpl::submitTask(Task* task) noexcept {
    LockGuard lock(mutex_);
    queue_.pushBack(task);
    condVar_.signal();
}

void ThreadPoolImpl::join() noexcept {
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

void ThreadPoolImpl::workerLoop() {
    LockGuard lock(mutex_);

    for (;; condVar_.wait(mutex_)) {
        while (auto t = (Task*)queue_.popFrontOrNull()) {
            UnlockGuard unlock(mutex_);

            t->run();
        }
    }
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
        struct Worker: public Runable, public WaitQueue::Item {
            WorkStealingThreadPool* pool_;
            u32 myIndex_;
            PCG32 rng_;
            Vector<u32> so_;
            Mutex mutex_;
            CondVar condVar_;
            IntrusiveList tasks_;
            IntrusiveList local_;
            IntMap<void*> tls_;
            Thread thread_;

            Worker(WorkStealingThreadPool* pool, u32 myIndex, u32 numWorkers);

            auto key() const noexcept {
                return thread_.threadId();
            }

            void flush() noexcept;

            Task* popNoLock() noexcept {
                return (Task*)tasks_.popFrontOrNull();
            }

            void push(Task* task) noexcept {
                LockGuard lock(mutex_);
                flush();
                tasks_.pushBack(task);
                condVar_.signal();
            }

            void pushLocal(Task* task) noexcept {
                {
                    LockGuard lock(mutex_);

                    tasks_.pushBack(task);
                }

                stdAtomicAddAndFetch(&pool_->epoch_, 1, MemoryOrder::Release);
                pool_->notifyOne();
            }

            void pushThrLocal(Task* task) noexcept {
                // pushLocal(task);
                local_.pushBack(task);
            }

            void notify() noexcept {
                LockGuard lock(mutex_);
                condVar_.signal();
            }

            void join() noexcept {
                thread_.join();
            }

            void sleep() noexcept {
                pool_->wq->enqueue(this);
                condVar_.wait(mutex_);
            }

            void loop();
            void run() noexcept override;
            void split(IntrusiveList* stolen) noexcept;
        };

        Vector<Worker*> workers_;
        IntMap<Worker> workerIndex_;
        WaitQueue::Ref wq;
        size_t running_;
        size_t epoch_;

        WorkStealingThreadPool(size_t numThreads);

        bool notifyOne() noexcept;
        void join() noexcept override;
        Worker* localWorker() noexcept;
        Worker* nextSleeping() noexcept;
        void** tls(u64 key) noexcept override;
        void submitTask(Task* task) noexcept override;
        void trySteal(const u32* order, u32 n, u32 offset, IntrusiveList* stolen) noexcept;
    };
}

WorkStealingThreadPool::WorkStealingThreadPool(size_t numThreads)
    : workers_(numThreads)
    , wq(WaitQueue::construct(numThreads))
    , running_(numThreads)
    , epoch_(0)
{
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.pushBack(workerIndex_.insertKeyed(this, i, numThreads));
    }

    for (auto w : mutRange(workers_)) {
        w->mutex_.unlock();
    }
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
    } else if (auto w = (Worker*)wq->dequeue()) {
        return w->push(task);
    } else {
        return workers_[PCG32(&task).uniformUnbiased(workers_.length())]->pushLocal(task);
    }
}

void** WorkStealingThreadPool::tls(u64 key) noexcept {
    if (auto w = localWorker(); w) {
        return &w->tls_[key];
    }

    return nullptr;
}

void WorkStealingThreadPool::join() noexcept {
    ShutDown task;

    submitTask(&task);

    for (auto w : mutRange(workers_)) {
        w->join();
    }
}

void WorkStealingThreadPool::trySteal(const u32* order, u32 n, u32 offset, IntrusiveList* stolen) noexcept {
    for (u32 i = 0; stolen->empty() && i < n; ++i) {
        workers_[order[(offset + i) % n]]->split(stolen);
    }
}

WorkStealingThreadPool::Worker* WorkStealingThreadPool::nextSleeping() noexcept {
    while (stdAtomicFetch(&running_, MemoryOrder::Relaxed) > 1) {
        if (auto w = (Worker*)wq->dequeue(); w) {
            return w;
        }
    }

    return nullptr;
}

WorkStealingThreadPool::Worker::Worker(WorkStealingThreadPool* pool, u32 myIndex, u32 numWorkers)
    : WaitQueue::Item{nullptr, (u8)myIndex}
    , pool_(pool)
    , myIndex_(myIndex)
    , rng_(splitMix64(myIndex))
    , so_(numWorkers - 1)
    , mutex_(true)
    , thread_(*this)
{
    for (u32 i = 0; i < numWorkers; ++i) {
        if (i != myIndex) {
            so_.pushBack(i);
        }
    }

    shuffle(rng_, so_.mutBegin(), so_.mutEnd());
}

void WorkStealingThreadPool::Worker::flush() noexcept {
    if (!local_.empty()) {
        tasks_.pushBack(local_);
        pool_->notifyOne();
    }
}

void WorkStealingThreadPool::Worker::run() noexcept {
    try {
        loop();
    } catch (ShutDown* sh) {
        if (auto w = pool_->nextSleeping(); w) {
            w->push(sh);
        }

        LockGuard lock(mutex_);

        flush();

        while (auto task = popNoLock()) {
            task->run();
        }

        stdAtomicSubAndFetch(&pool_->running_, 1, MemoryOrder::Relaxed);
    }
}

void WorkStealingThreadPool::Worker::loop() {
    LockGuard lock(mutex_);

    size_t epoch = 0;

    do {
        flush();

        while (auto task = popNoLock()) {
            {
                UnlockGuard unlock(mutex_);

                task->run();
            }

            flush();
        }

        epoch = stdAtomicFetch(&pool_->epoch_, MemoryOrder::Acquire);

        IntrusiveList stolen;

        {
            UnlockGuard unlock(mutex_);

            pool_->trySteal(so_.data(), (u32)so_.length(), rng_.uniformUnbiased(so_.length()), &stolen);
        }

        local_.pushBack(stolen);

        flush();
    } while (!tasks_.empty() || epoch != stdAtomicFetch(&pool_->epoch_, MemoryOrder::Acquire) || (sleep(), true));
}

void WorkStealingThreadPool::Worker::split(IntrusiveList* stolen) noexcept {
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
