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
#include <std/alg/defer.h>
#include <std/alg/minmax.h>
#include <std/lib/vector.h>
#include <std/sys/atomic.h>
#include <std/ptr/scoped.h>
#include <std/alg/exchange.h>
#include <std/mem/obj_pool.h>

using namespace Std;

namespace {
    struct ShutDown: public Task {
        inline ~ShutDown() noexcept {
            unlink();
        }

        void run() override {
            throw this;
        }
    };

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
                try {
                    pool_->workerLoop();
                } catch (ShutDown* sh) {
                    pool_->submitTask(*sh);
                }
            }
        };

        ObjPool::Ref pool_ = ObjPool::fromMemory();
        Mutex mutex_;
        CondVar condVar_;
        IntrusiveList queue_;
        IntrusiveList workers_;

        void workerLoop();

    public:
        ThreadPoolImpl(size_t numThreads);

        void submitTask(Task& task) noexcept override;
        void join() noexcept override;
    };
}

ThreadPoolImpl::ThreadPoolImpl(size_t numThreads) {
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
    ShutDown task;

    submitTask(task);

    for (auto node = workers_.mutFront(), end = workers_.mutEnd(); node != end; node = node->next) {
        static_cast<Worker*>(node)->thread_.join();
    }
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
                , mutex_(true)
                , thread_(*this)
            {
            }

            inline auto key() const noexcept {
                return thread_.threadId();
            }

            inline Task* popNoLock() noexcept {
                return (Task*)tasks_.popFrontOrNull();
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

            inline void pushLocal(Task& task) noexcept {
                LockGuard lock(mutex_);
                tasks_.pushBack(&task);
            }

            inline void signal() noexcept {
                // LockGuard lock(mutex_);
                condVar_.signal();
            }

            inline bool tryPush(Task& task) noexcept {
                LockGuard lock(mutex_);

                if (pool_) {
                    tasks_.pushBack(&task);
                    condVar_.signal();

                    return true;
                } else {
                    return false;
                }
            }

            inline void join() noexcept {
                thread_.join();
            }

            void loop();
            void run() noexcept override;
            void stealInto(IntrusiveList* stolen) noexcept;
        };

        Vector<Worker*> workers_;
        unsigned int nextWorker_ = 0;
        IntMap<Worker> workerIndex_;

        void trySteal(PCG32& rng, IntrusiveList* stolen) noexcept;

        inline auto nextWorker() noexcept {
            return workers_[stdAtomicAddAndFetch(&nextWorker_, 1, MemoryOrder::Relaxed) % workers_.length()];
        }

    public:
        WorkStealingThreadPool(size_t numThreads);

        void submitTask(Task& task) noexcept override;
        void join() noexcept override;
    };
}

WorkStealingThreadPool::WorkStealingThreadPool(size_t numThreads)
    : workers_(numThreads)
{
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.pushBack(workerIndex_.insertKeyed(this));
    }

    for (auto w : mutRange(workers_)) {
        w->mutex_.unlock();
    }
}

void WorkStealingThreadPool::submitTask(Task& task) noexcept {
    if (auto w = workerIndex_.find(Thread::currentThreadId()); w) {
        STD_DEFER {
            nextWorker()->signal();
        };

        return w->pushLocal(task);
    }

    nextWorker()->push(task);
}

void WorkStealingThreadPool::join() noexcept {
    ShutDown task;

    submitTask(task);

    for (auto w : mutRange(workers_)) {
        w->join();
    }
}

void WorkStealingThreadPool::Worker::run() noexcept {
    auto pool = pool_;

    try {
        loop();
    } catch (ShutDown* sh) {
        STD_DEFER {
            LockGuard lock(mutex_);

            while (auto task = popNoLock()) {
                task->run();
            }
        };

        for (auto w : mutRange(pool->workers_)) {
            if (w->tryPush(*sh)) {
                return;
            }
        }
    }
}

void WorkStealingThreadPool::Worker::loop() {
    LockGuard lock(mutex_);

    STD_DEFER {
        pool_ = nullptr;
    };

    do {
        while (auto task = popNoLock()) {
            UnlockGuard unlock(mutex_);

            task->run();
        }

        IntrusiveList stolen;

        {
            UnlockGuard unlock(mutex_);

            pool_->trySteal(rng_, &stolen);
        }

        tasks_.pushBack(stolen);
    } while (!tasks_.empty() || (condVar_.wait(mutex_), true));
}

void WorkStealingThreadPool::Worker::stealInto(IntrusiveList* stolen) noexcept {
    LockGuard lock(mutex_);

    tasks_.splitHalf(tasks_, *stolen);

    if (!tasks_.empty()) {
        pool_->nextWorker()->signal();
    }
}

void WorkStealingThreadPool::trySteal(PCG32& rng, IntrusiveList* stolen) noexcept {
    for (size_t numWorkers = workers_.length(), i = 0; stolen->empty() && i < numWorkers; ++i) {
        workers_[rng.uniformBiased(numWorkers)]->stealInto(stolen);
    }
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
