#include "thread_pool.h"

#include "mutex.h"
#include "thread.h"
#include "cond_var.h"

#include <std/lib/list.h>
#include <std/lib/vector.h>
#include <std/mem/obj_pool.h>

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

            task = static_cast<Task*>(queue_.popFront());
        }

        task->run();
    }
}

ThreadPool::~ThreadPool() noexcept {
}

ThreadPool::Ref ThreadPool::simple(size_t threads) {
    return new ThreadPoolImpl(threads);
}
