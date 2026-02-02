#pragma once

#include "thread.h"
#include "mutex.h"
#include "cond_var.h"

#include <std/lib/list.h>
#include <std/lib/vector.h>
#include <std/mem/obj_pool.h>

namespace Std {
    struct Task : public Runable, public IntrusiveNode {
    };

    class ThreadPool {
        struct Worker : public Runable {
            ThreadPool* pool_;
            
            explicit Worker(ThreadPool* p) noexcept
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

    public:
        explicit ThreadPool(size_t numThreads);
        ~ThreadPool() noexcept;
        
        void submit(Task* task);
        void join() noexcept;
    };
}
