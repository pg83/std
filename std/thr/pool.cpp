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
        struct LocalWorker {
            virtual void pushThrLocal(Task* task) noexcept = 0;
            virtual void** tls(u64 key) noexcept = 0;
        };

        struct Worker: public LocalWorker, public Runable, public WaitQueue::Item {
            WorkStealingThreadPool* pool_;
            PCG32 rng_;
            Vector<Worker*> stealOrder_;
            Mutex mutex_;
            CondVar condVar_;
            IntrusiveList tasks_;
            IntrusiveList local_;
            IntMap<void*> tls_;
            Thread thread_;

            Worker(WorkStealingThreadPool* pool, u32 myIndex);

            auto key() const noexcept {
                return thread_.threadId();
            }

            void flushLocal() noexcept {
                tasks_.pushBack(local_);
            }

            Task* popNoLock() noexcept {
                return (Task*)tasks_.popFrontOrNull();
            }

            template <typename T>
            void push(T& task) noexcept {
                LockGuard lock(mutex_);
                flushLocal();
                tasks_.pushBack(task);
                condVar_.signal();
            }

            void pushThrLocal(Task* task) noexcept override {
                local_.pushBack(task);
            }

            void** tls(u64 key) noexcept override {
                return &tls_[key];
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

            void initStealOrder() noexcept;
            void trySteal(IntrusiveList* stolen) noexcept;
            void loop();
            void run() noexcept override;
            void split(IntrusiveList* stolen) noexcept;
        };

        struct GlobalWorker: public LocalWorker, public Runable {
            WorkStealingThreadPool* pool_;
            Mutex mutex_;
            CondVar condVar_;
            IntrusiveList tasks_;
            IntMap<void*> tls_;
            Thread thread_;
            bool done_ = false;

            explicit GlobalWorker(WorkStealingThreadPool* pool) noexcept;

            void push(Task* task) noexcept;

            void pushThrLocal(Task* task) noexcept override {
                push(task);
            }

            void** tls(u64 key) noexcept override {
                return &tls_[key];
            }

            void steal(IntrusiveList* stolen) noexcept {
                LockGuard lock(mutex_);
                tasks_.xchgWithEmptyList(*stolen);
            }

            void loop();
            void run() noexcept override;

            void join() {
                {
                    LockGuard g(mutex_);
                    done_ = true;
                    condVar_.signal();
                }

                thread_.join();
            }
        };

        IntMap<Worker> workerIndex_;
        WaitQueue::Ref wq;
        size_t running_;
        GlobalWorker gw_;

        WorkStealingThreadPool(size_t numThreads);

        bool notifyOne() noexcept;
        void join() noexcept override;
        LocalWorker* localWorker() noexcept;
        void** tls(u64 key) noexcept override;
        void submitTask(Task* task) noexcept override;
    };
}

WorkStealingThreadPool::GlobalWorker::GlobalWorker(WorkStealingThreadPool* pool) noexcept
    : pool_(pool)
    , thread_(*this)
{
}

void WorkStealingThreadPool::GlobalWorker::push(Task* task) noexcept {
    LockGuard lock(mutex_);
    tasks_.pushBack(task);
    condVar_.signal();
}

void WorkStealingThreadPool::GlobalWorker::loop() {
    LockGuard lock(mutex_);

    while (!done_) {
        while (!done_ && tasks_.empty()) {
            condVar_.wait(mutex_);
        }

        if (auto w = (Worker*)pool_->wq->dequeue()) {
            IntrusiveList tmp;

            tasks_.xchgWithEmptyList(tmp);

            {
                UnlockGuard unlock(mutex_);

                w->push(tmp);
            }
        } else if (auto task = (Task*)tasks_.popFrontOrNull(); task) {
            UnlockGuard unlock(mutex_);

            task->run();
        }
    }
}

void WorkStealingThreadPool::GlobalWorker::run() noexcept {
    loop();
}

WorkStealingThreadPool::WorkStealingThreadPool(size_t numThreads)
    : wq(WaitQueue::construct(numThreads))
    , running_(numThreads)
    , gw_(this)
{
    for (size_t i = 0; i < numThreads; ++i) {
        workerIndex_.insertKeyed(this, (u32)i);
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

WorkStealingThreadPool::LocalWorker* WorkStealingThreadPool::localWorker() noexcept {
    static thread_local LocalWorker* curw = nullptr;

    if (curw) {
        return curw;
    } else if (auto w = workerIndex_.find(Thread::currentThreadId()); w) {
        return curw = w;
    } else if (gw_.thread_.threadId() == Thread::currentThreadId()) {
        return curw = &gw_;
    }

    return nullptr;
}

void WorkStealingThreadPool::submitTask(Task* task) noexcept {
    if (auto w = localWorker(); w) {
        return w->pushThrLocal(task);
    } else if (auto w = (Worker*)wq->dequeue()) {
        return w->push(task);
    } else {
        return gw_.push(task);
    }
}

void** WorkStealingThreadPool::tls(u64 key) noexcept {
    if (auto w = localWorker()) {
        return w->tls(key);
    }

    return nullptr;
}

void WorkStealingThreadPool::join() noexcept {
    while (running_) {
        if (auto w = (Worker*)wq->dequeue(); w) {
            ShutDown sh;

            w->pushThrLocal(&sh);
            w->notify();
            w->join();
        }
    }

    gw_.join();
}

WorkStealingThreadPool::Worker::Worker(WorkStealingThreadPool* pool, u32 myIndex)
    : WaitQueue::Item{nullptr, (u8)myIndex}
    , pool_(pool)
    , rng_(splitMix64(myIndex))
    , mutex_(true)
    , thread_(*this)
{
}

void WorkStealingThreadPool::Worker::initStealOrder() noexcept {
    pool_->workerIndex_.visit([this](Worker& w) {
        if (&w != this) {
            stealOrder_.pushBack(&w);
        }
    });

    shuffle(rng_, stealOrder_.mutBegin(), stealOrder_.mutEnd());
}

void WorkStealingThreadPool::Worker::trySteal(IntrusiveList* stolen) noexcept {
    pool_->gw_.steal(stolen);

    u32 n = (u32)stealOrder_.length();
    u32 offset = rng_.uniformUnbiased(n);

    for (u32 i = 0; stolen->empty() && i < n; ++i) {
        stealOrder_[(offset + i) % n]->split(stolen);
    }
}

void WorkStealingThreadPool::Worker::run() noexcept {
    try {
        loop();
    } catch (ShutDown* sh) {
    }

    LockGuard lock(mutex_);

    flushLocal();

    while (auto task = popNoLock()) {
        task->run();
        flushLocal();
    }

    stdAtomicSubAndFetch(&pool_->running_, 1, MemoryOrder::Relaxed);
}

void WorkStealingThreadPool::Worker::loop() {
    LockGuard lock(mutex_);

    do {
        // ShutDown task only
        while (auto task = (Task*)local_.popFrontOrNull()) {
            task->run();
        }

        while (auto task = popNoLock()) {
            pool_->notifyOne();

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
