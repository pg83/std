#include "pool.h"
#include "task.h"
#include "mutex.h"
#include "guard.h"
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
#include <std/mem/obj_pool.h>

#include <stdlib.h>
#include <sched.h>

using namespace stl;

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
        IntMap<Worker> workers_;
        size_t inflight_ = 0;

        void workerLoop();

    public:
        ThreadPoolImpl(size_t numThreads);
        ~ThreadPoolImpl() noexcept;

        void submitTask(Task* task) noexcept override;
        void join() noexcept override;
        void** tls(u64 key) noexcept override;
        PCG32& random() noexcept override;
    };
}

ThreadPoolImpl::ThreadPoolImpl(size_t numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.insertKeyed(this);
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

    workers_.visit([](Worker& w) {
        w.thread_.join();
    });
}

void** ThreadPoolImpl::tls(u64 key) noexcept {
    if (auto w = workers_.find(Thread::currentThreadId()); w) {
        return &w->tls_[key];
    }

    return nullptr;
}

PCG32& ThreadPoolImpl::random() noexcept {
    return workers_.find(Thread::currentThreadId())->rng_;
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

ThreadPool::~ThreadPool() noexcept {
}

ThreadPool* ThreadPool::sync(ObjPool* pool) {
    return pool->make<SyncThreadPool>();
}

ThreadPool* ThreadPool::simple(ObjPool* pool, size_t threads) {
    if (threads == 0) {
        return sync(pool);
    }

    return pool->make<ThreadPoolImpl>(threads);
}

namespace {
    struct WorkStealingThreadPool: public ThreadPool {
        struct Worker: public Runable, public WaitQueue::Item {
            WorkStealingThreadPool* pool_;
            IntMap<void*> tls_;
            PCG32 rng_;
            Vector<Worker*> so_;
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

            void** tls(u64 key) noexcept {
                return &tls_[key];
            }

            PCG32& random() noexcept {
                return rng_;
            }

            void loop();
            void join() noexcept;
            void sleep() noexcept;
            void run() noexcept override;
            void initStealOrder() noexcept;
            void push(Task* task) noexcept;
            void pushThrLocal(Task* task) noexcept;
            void push(IntrusiveList* task) noexcept;
            bool shouldSleep(i32 searching) noexcept;
            void steal(IntrusiveList* stolen) noexcept;
            void trySteal(IntrusiveList* stolen) noexcept;
            void splitHalf(IntrusiveList* stolen) noexcept;
        };

        IntMap<Worker> workers_;
        WaitQueue* wq;
        i32 taskCount_ = 0;
        i32 searching_ = 0;

        WorkStealingThreadPool(ObjPool* pool, size_t numThreads);
        ~WorkStealingThreadPool() noexcept;

        void join() noexcept override;
        Worker* localWorker() noexcept;
        Worker* dequeueWorker() noexcept;
        PCG32& random() noexcept override;
        void** tls(u64 key) noexcept override;
        void submitTask(Task* task) noexcept override;
    };
}

void WorkStealingThreadPool::Worker::pushThrLocal(Task* task) noexcept {
    if (task->priority()) {
        local_.pushFront(task);
    } else {
        local_.pushBack(task);
    }
}

void WorkStealingThreadPool::Worker::sleep() noexcept {
    pool_->wq->enqueue(this);
    condVar_.wait(mutex_);
}

void WorkStealingThreadPool::Worker::push(Task* task) noexcept {
    LockGuard lock(mutex_);
    tasks_.pushBack(task);
    condVar_.signal();
}

void WorkStealingThreadPool::Worker::push(IntrusiveList* tasks) noexcept {
    LockGuard lock(mutex_);
    tasks_.pushBack(*tasks);
    condVar_.signal();
}

WorkStealingThreadPool::WorkStealingThreadPool(ObjPool* pool, size_t numThreads)
    : wq(WaitQueue::construct(pool, numThreads))
{
    PCG32 rng(this);

    for (size_t i = 0; i < numThreads; ++i) {
        workers_.insertKeyed(this, i, rng.nextU64());
    }

    workers_.visit([](Worker& w) {
        w.initStealOrder();
        w.mutex_.unlock();
    });
}

WorkStealingThreadPool::Worker* WorkStealingThreadPool::dequeueWorker() noexcept {
    while (true) {
        if (auto w = (Worker*)wq->dequeue(); w) {
            return w;
        }

        sched_yield();
    }
}

WorkStealingThreadPool::Worker* WorkStealingThreadPool::localWorker() noexcept {
    static thread_local Worker* curw = nullptr;

    if (curw) {
        return curw;
    } else if (auto w = workers_.find(Thread::currentThreadId()); w) {
        return curw = w;
    }

    return nullptr;
}

void WorkStealingThreadPool::submitTask(Task* task) noexcept {
    stdAtomicAddAndFetch(&taskCount_, 1, MemoryOrder::Release);

    if (auto w = localWorker(); w) {
        return w->pushThrLocal(task);
    } else {
        return dequeueWorker()->push(task);
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
    while (wq->sleeping() != workers_.size()) {
        sched_yield();
    }
}

WorkStealingThreadPool::~WorkStealingThreadPool() noexcept {
    join();

    workers_.visit([](Worker& w) {
        w.join();
    });

    STD_INSIST(taskCount_ == 0);
}

WorkStealingThreadPool::Worker::Worker(WorkStealingThreadPool* pool, u32 myIndex, u64 seed)
    : WaitQueue::Item{nullptr, (u8)myIndex}
    , pool_(pool)
    , rng_(seed)
    , mutex_(true)
    , thread_(*this)
{
}

void WorkStealingThreadPool::Worker::join() noexcept {
    ShutDown sh;
    stdAtomicAddAndFetch(&pool_->taskCount_, 1, MemoryOrder::Relaxed);
    push(&sh);
    thread_.join();
}

void WorkStealingThreadPool::Worker::initStealOrder() noexcept {
    pool_->workers_.visit([this](Worker& w) {
        if (&w != this) {
            so_.pushBack(&w);
        }
    });

    shuffle(rng_, so_.mutBegin(), so_.mutEnd());
}

bool WorkStealingThreadPool::Worker::shouldSleep(i32 searching) noexcept {
    if (!tasks_.empty()) {
        return false;
    }

    if (searching == 0) {
        if (stdAtomicFetch(&pool_->taskCount_, MemoryOrder::Acquire) > 0) {
            return false;
        }
    }

    return true;
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

    while (true) {
        while (auto task = popNoLock()) {
            if (tasks_.empty()) {
                // pass
            } else if (stdAtomicFetch(&pool_->searching_, MemoryOrder::Acquire)) {
                // pass
            } else if (auto w = (Worker*)pool_->wq->dequeue(); w) {
                w->push(popNoLock());
            }

            stdAtomicSubAndFetch(&pool_->taskCount_, 1, MemoryOrder::Relaxed);

            UnlockGuard(mutex_).run([task] {
                task->run();
            });

            flushLocal();
        }

        stdAtomicAddAndFetch(&pool_->searching_, 1, MemoryOrder::Relaxed);

        IntrusiveList stolen;

        UnlockGuard(mutex_).run([this, &stolen] {
            trySteal(&stolen);
        });

        local_.pushBack(stolen);
        flushLocal();

        if (shouldSleep(stdAtomicSubAndFetch(&pool_->searching_, 1, MemoryOrder::Release))) {
            sleep();
            flushLocal();
        }
    }
}

void WorkStealingThreadPool::Worker::steal(IntrusiveList* stolen) noexcept {
    LockGuard lock(mutex_);
    splitHalf(stolen);
}

void WorkStealingThreadPool::Worker::splitHalf(IntrusiveList* stolen) noexcept {
    tasks_.splitHalf(tasks_, *stolen);

    if (stolen->empty()) {
        tasks_.xchgWithEmptyList(*stolen);
    }
}

ThreadPool* ThreadPool::workStealing(ObjPool* pool, size_t threads) {
    if (threads <= 1) {
        return simple(pool, threads);
    }

    return pool->make<WorkStealingThreadPool>(pool, threads);
}

u64 ThreadPool::registerTlsKey() noexcept {
    static u64 tlsKeyCounter = 0;

    return stdAtomicAddAndFetch(&tlsKeyCounter, 1, MemoryOrder::Relaxed);
}
