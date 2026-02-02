#include "thread_pool.h"

using namespace Std;

ThreadPool::ThreadPool(size_t numThreads)
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

ThreadPool::~ThreadPool() noexcept {
}

void ThreadPool::submit(Task* task) {
    LockGuard lock(mutex_);
    queue_.pushBack(task);
    condVar_.signal();
}

void ThreadPool::join() noexcept {
    {
        LockGuard lock(mutex_);
        shutdown_ = true;
        condVar_.broadcast();
    }
    
    for (size_t i = 0; i < numThreads_; ++i) {
        threads_[i]->join();
    }
}

void ThreadPool::workerLoop() noexcept {
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

void ThreadPool::Worker::run() noexcept {
    pool_->workerLoop();
}
