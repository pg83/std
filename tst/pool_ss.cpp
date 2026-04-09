#include <std/thr/pool.h>
#include <std/thr/mutex.h>
#include <std/thr/guard.h>
#include <std/thr/cond_var.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>

using namespace stl;

namespace {
    void doW(int work) {
        for (volatile int i = 0; i < work; i = i + 1) {
        }
    }

    struct StressState {
        ThreadPool* pool;
        int work;
        int counter;
        Mutex mutex;
        CondVar condVar;

        StressState(ThreadPool* p, int w)
            : pool(p)
            , work(w)
            , counter(1)
        {
        }
    };

    struct StressTask {
        StressState* state_;
        int depth_;

        StressTask(StressState* s, int d)
            : state_(s)
            , depth_(d)
        {
        }

        void doWork() noexcept {
            doW(state_->work);
        }

        void schedule() {
            stdAtomicAddAndFetch(&state_->counter, 1, MemoryOrder::Relaxed);
            auto t = new StressTask(state_, depth_ - 1);
            state_->pool->submit([t] {
                t->run();
            });
        }

        void run() noexcept {
            doWork();
            if (depth_ > 0) {
                schedule();
            }
            doWork();
            if (depth_ > 0) {
                schedule();
            }
            doWork();

            auto state = state_;
            delete this;

            if (stdAtomicSubAndFetch(&state->counter, 1, MemoryOrder::Release) == 0) {
                LockGuard lock(state->mutex);
                state->condVar.signal();
            }
        }
    };
}

int main() {
    auto opool = ObjPool::fromMemory();
    auto pool = ThreadPool::simple(opool.mutPtr(), 16);
    StressState state(pool, 250);

    auto task = new StressTask(&state, 25);
    pool->submit([task] {
        task->run();
    });

    {
        LockGuard lock(state.mutex);

        while (stdAtomicFetch(&state.counter, MemoryOrder::Acquire) > 0) {
            state.condVar.wait(state.mutex);
        }
    }

    pool->join();
}
