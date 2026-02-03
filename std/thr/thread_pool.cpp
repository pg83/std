#include "thread_pool.h"

#include "mutex.h"
#include "thread.h"
#include "cond_var.h"

#include <std/lib/list.h>
#include <std/lib/vector.h>
#include <std/mem/obj_pool.h>
#include <std/sys/atomic.h>

using namespace Std;

namespace {
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

        void submit(Task& task) override;
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

void ThreadPoolImpl::submit(Task& task) {
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
    while (true) {
        Task* task = nullptr;

        {
            LockGuard lock(mutex_);

            while (queue_.empty() && !shutdown_) {
                condVar_.wait(mutex_);
            }

            if (shutdown_ && queue_.empty()) {
                return;
            }

            if (!queue_.empty()) {
                task = static_cast<Task*>(queue_.popFront());
            }
        }

        if (task) {
            task->run();
        }
    }
}

ThreadPool::~ThreadPool() noexcept {
}

ThreadPool::Ref ThreadPool::simple(size_t threads) {
    return new ThreadPoolImpl(threads);
}

namespace {
    class WorkStealingThreadPool: public ThreadPool {
        struct Worker: public Runable {
            WorkStealingThreadPool* pool_;
            size_t workerId_;
            Mutex mutex_;
            CondVar condVar_;
            IntrusiveList tasks_;
            Thread thread_;

            inline Worker(WorkStealingThreadPool* pool, size_t id) noexcept
                : pool_(pool)
                , workerId_(id)
                , thread_(*this)
            {
            }

            inline Task* pop() noexcept {
                LockGuard lock(mutex_);
                if (tasks_.empty()) {
                    return nullptr;
                }
                return static_cast<Task*>(tasks_.popBack());
            }

            inline void push(Task& task) noexcept {
                LockGuard lock(mutex_);
                tasks_.pushBack(&task);
                condVar_.signal();
            }

            Task* next() noexcept;

            void run() noexcept override;
        };

        int shutdown_;
        ObjPool::Ref pool_;
        Vector<Worker*> workers_;
        int nextWorker_;

        Task* tryStealTask(size_t myId) noexcept;

    public:
        WorkStealingThreadPool(size_t numThreads);

        void submit(Task& task) override;
        void join() noexcept override;
    };
}

WorkStealingThreadPool::WorkStealingThreadPool(size_t numThreads)
    : shutdown_(0)
    , pool_(ObjPool::fromMemory())
    , nextWorker_(0)
{
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.pushBack(pool_->make<Worker>(this, i));
    }
}

void WorkStealingThreadPool::submit(Task& task) {
    int idx = stdAtomicAddAndFetch(&nextWorker_, 1, MemoryOrder::Relaxed);
    size_t workerIdx = static_cast<size_t>(idx) % workers_.length();
    workers_[workerIdx]->push(task);
}

void WorkStealingThreadPool::join() noexcept {
    stdAtomicStore(&shutdown_, 1, MemoryOrder::Release);

    for (size_t i = 0; i < workers_.length(); ++i) {
        workers_[i]->condVar_.signal();
    }

    for (size_t i = 0; i < workers_.length(); ++i) {
        workers_[i]->thread_.join();
    }

    for (size_t i = 0; i < workers_.length(); ++i) {
        Task* task;

        while ((task = workers_[i]->pop()) != nullptr) {
            task->run();
        }
    }
}

Task* WorkStealingThreadPool::Worker::next() noexcept {
    if (auto task = pop(); task) {
        return task;
    }

    return pool_->tryStealTask(workerId_);
}

void WorkStealingThreadPool::Worker::run() noexcept {
    while (true) {
        if (auto task = next(); task) {
            task->run();
        } else {
            if (stdAtomicFetch(&pool_->shutdown_, MemoryOrder::Acquire)) {
                return;
            }

            LockGuard lock(mutex_);

            if (stdAtomicFetch(&pool_->shutdown_, MemoryOrder::Acquire)) {
                return;
            }

            condVar_.wait(mutex_);
        }
    }
}

Task* WorkStealingThreadPool::tryStealTask(size_t myId) noexcept {
    size_t numWorkers = workers_.length();

    if (numWorkers <= 1) {
        return nullptr;
    }

    for (size_t i = 1; i < numWorkers; ++i) {
        if (auto task = workers_[(myId + i) % numWorkers]->pop(); task) {
            return task;
        }
    }

    return nullptr;
}

ThreadPool::Ref ThreadPool::workStealing(size_t threads) {
    return static_cast<ThreadPool*>(new WorkStealingThreadPool(threads));
}
