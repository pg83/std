#include "thread_pool.h"

#include "mutex.h"
#include "thread.h"
#include "cond_var.h"

#include <std/lib/list.h>
#include <std/lib/vector.h>
#include <std/mem/obj_pool.h>

using namespace Std;

namespace {
    struct ThreadPoolImpl: public ThreadPool {
        struct Worker: public Runable {
            ThreadPoolImpl* pool_;

            explicit Worker(ThreadPoolImpl* p) noexcept
                : pool_(p)
            {
            }

            void run() noexcept override;
        };

        Mutex mutex_;
        CondVar condVar_;
        IntrusiveList queue_;
        bool shutdown_;

        ObjPool::Ref pool_;
        Vector<Thread*> threads_;
        size_t numThreads_;

        void workerLoop() noexcept;

        explicit ThreadPoolImpl(size_t numThreads);
        ~ThreadPoolImpl() noexcept;

        void submit(Task& task) override;
        void join() noexcept override;
    };
}

ThreadPoolImpl::ThreadPoolImpl(size_t numThreads)
    : shutdown_(false)
    , pool_(ObjPool::fromMemory())
    , numThreads_(numThreads)
{
    threads_.grow(numThreads);

    for (size_t i = 0; i < numThreads_; ++i) {
        Worker* worker = pool_->make<Worker>(this);
        threads_.mut(i) = pool_->make<Thread>(*worker);
    }
}

ThreadPoolImpl::~ThreadPoolImpl() noexcept {
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

    for (size_t i = 0; i < numThreads_; ++i) {
        threads_[i]->join();
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

            task = static_cast<Task*>(queue_.popFront());
        }

        task->run();
    }
}

void ThreadPoolImpl::Worker::run() noexcept {
    pool_->workerLoop();
}

ThreadPool::~ThreadPool() noexcept {
}

ThreadPool::Ref ThreadPool::simple(size_t threads) {
    return new ThreadPoolImpl(threads);
}
