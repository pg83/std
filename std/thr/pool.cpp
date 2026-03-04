#include "pool.h"
#include "task.h"
#include "mutex.h"
#include "thread.h"
#include "runable.h"
#include "cond_var.h"

#include <std/rng/pcg.h>
#include <std/lib/list.h>
#include <std/sym/i_map.h>
#include <std/alg/range.h>
#include <std/lib/vector.h>
#include <std/sys/atomic.h>
#include <std/ptr/scoped.h>
#include <std/mem/obj_pool.h>

using namespace Std;

namespace {
    struct SyncThreadPool: public ThreadPool {
        void submitTask(Task& task) noexcept override {
            task.run();
        }

        void join() noexcept override {
        }
    };

    class ThreadPoolImpl: public ThreadPool {
        struct Worker: public Runable, public IntrusiveNode {
            ThreadPoolImpl* pool_;
            Thread thread_;

            inline explicit Worker(ThreadPoolImpl* p) noexcept
                : pool_(p)
                , thread_(*this)
            {
            }

            void run() noexcept override {
                pool_->workerLoop();
            }
        };

        Mutex mutex_;
        CondVar condVar_;
        IntrusiveList queue_;
        bool shutdown_;

        ObjPool::Ref pool_;
        IntrusiveList workers_;

        void workerLoop() noexcept;

    public:
        ThreadPoolImpl(size_t numThreads);

        void submitTask(Task& task) noexcept override;
        void join() noexcept override;
    };
}

ThreadPoolImpl::ThreadPoolImpl(size_t numThreads)
    : shutdown_(false)
    , pool_(ObjPool::fromMemory())
{
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.pushBack(pool_->make<Worker>(this));
    }
}

void ThreadPoolImpl::submitTask(Task& task) noexcept {
    LockGuard lock(mutex_);
    queue_.pushBack(&task);
    condVar_.signal();
}

void ThreadPoolImpl::join() noexcept {
    {
        LockGuard lock(mutex_);
        shutdown_ = true;
        condVar_.broadcast();
    }

    for (auto node = workers_.mutFront(), end = workers_.mutEnd(); node != end; node = node->next) {
        static_cast<Worker*>(node)->thread_.join();
    }
}

void ThreadPoolImpl::workerLoop() noexcept {
    LockGuard lock(mutex_);

    do {
        while (auto t = (Task*)queue_.popFrontOrNull()) {
            UnlockGuard unlock(mutex_);

            t->run();
        }
    } while (!shutdown_ && (condVar_.wait(mutex_), true));
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
    class WorkStealingThreadPool: public ThreadPool {
        struct Worker: public Runable {
            WorkStealingThreadPool* pool_;
            PCG32 rng_;
            Mutex mutex_;
            CondVar condVar_;
            IntrusiveList tasks_;
            Thread thread_;

            inline Worker(WorkStealingThreadPool* pool)
                : pool_(pool)
                , rng_(this)
                , thread_(*this)
            {
            }

            inline auto key() const noexcept {
                return thread_.threadId();
            }

            inline Task* popNoLock() noexcept {
                return (Task*)tasks_.popBackOrNull();
            }

            inline Task* pop() noexcept {
                LockGuard lock(mutex_);

                return popNoLock();
            }

            inline void push(Task& task) noexcept {
                LockGuard lock(mutex_);
                tasks_.pushBack(&task);
                condVar_.signal();
            }

            inline void signal() noexcept {
                LockGuard lock(mutex_);
                condVar_.signal();
            }

            inline void join() noexcept {
                thread_.join();
            }

            void run() noexcept override;
        };

        int shutdown_;
        Vector<Worker*> workers_;
        unsigned int nextWorker_;
        IntMap<Worker> workerIndex_;

        Task* tryStealTask(PCG32& rng) noexcept;

    public:
        WorkStealingThreadPool(size_t numThreads);

        inline bool shutdown() const noexcept {
            return stdAtomicFetch(&shutdown_, MemoryOrder::Acquire);
        }

        void submitTask(Task& task) noexcept override;
        void join() noexcept override;
    };
}

WorkStealingThreadPool::WorkStealingThreadPool(size_t numThreads)
    : shutdown_(0)
    , nextWorker_(0)
{
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.pushBack(workerIndex_.insertKeyed(this));
    }
}

void WorkStealingThreadPool::submitTask(Task& task) noexcept {
    if (auto w = workerIndex_.find(Thread::currentThreadId()); w) {
        return w->push(task);
    }

    const size_t idx = stdAtomicAddAndFetch(&nextWorker_, 1, MemoryOrder::Relaxed);

    workers_[idx % workers_.length()]->push(task);
}

void WorkStealingThreadPool::join() noexcept {
    stdAtomicStore(&shutdown_, 1, MemoryOrder::Release);

    for (auto w : mutRange(workers_)) {
        w->signal();
    }

    for (auto w : mutRange(workers_)) {
        w->join();
    }
}

void WorkStealingThreadPool::Worker::run() noexcept {
    LockGuard lock(mutex_);

    do {
        while (auto task = popNoLock()) {
            do {
                UnlockGuard unlock(mutex_);

                task->run();
            } while (task = popNoLock());

            UnlockGuard unlock(mutex_);

            if (auto task = pool_->tryStealTask(rng_); task) {
                task->run();
            }
        }
    } while (!pool_->shutdown() && (condVar_.wait(mutex_), true));
}

Task* WorkStealingThreadPool::tryStealTask(PCG32& rng) noexcept {
    for (size_t numWorkers = workers_.length(), i = 0; i < numWorkers; ++i) {
        if (auto task = workers_[rng.uniformBiased(numWorkers)]->pop(); task) {
            return task;
        }
    }

    return nullptr;
}

ThreadPool::Ref ThreadPool::workStealing(size_t threads) {
    if (threads <= 1) {
        return simple(threads);
    }

    return new WorkStealingThreadPool(threads);
}

void ThreadPool::submit(Runable& runable) {
    struct Helper: public Task {
        Runable* runable;

        inline Helper(Runable* r) noexcept
            : runable(r)
        {
        }

        void run() noexcept override {
            ScopedPtr<Helper> self(this);
            runable->run();
        }
    };

    ScopedPtr<Helper> task(new Helper(&runable));

    submitTask(*task.ptr);
    task.drop();
}
