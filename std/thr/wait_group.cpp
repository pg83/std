#include "wait_group.h"

#include "coro.h"
#include "mutex.h"
#include "guard.h"
#include "cond_var.h"

#include <std/sys/types.h>
#include <std/mem/obj_pool.h>

using namespace stl;

namespace {
    struct WaitGroupImpl: public WaitGroup {
        Mutex* mutex;
        CondVar* cv;
        size_t counter;

        WaitGroupImpl(Mutex* m, CondVar* c, size_t init)
            : mutex(m)
            , cv(c)
            , counter(init)
        {
        }

        void add(size_t n) noexcept override {
            LockGuard lock(mutex);
            counter += n;
        }

        void done() noexcept override {
            LockGuard lock(mutex);

            if (--counter == 0) {
                cv->broadcast();
            }
        }

        void wait() noexcept override {
            LockGuard lock(mutex);

            while (counter > 0) {
                cv->wait(mutex);
            }
        }
    };
}

WaitGroup* WaitGroup::create(ObjPool* pool, size_t init) {
    return pool->make<WaitGroupImpl>(Mutex::create(pool), CondVar::create(pool), init);
}

WaitGroup* WaitGroup::create(ObjPool* pool, size_t init, CoroExecutor* exec) {
    return pool->make<WaitGroupImpl>(Mutex::create(pool, exec), CondVar::create(pool, exec), init);
}
