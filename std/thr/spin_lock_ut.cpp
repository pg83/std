#include "spin_lock.h"
#include "pool.h"

#include <std/tst/ut.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>

using namespace stl;

STD_TEST_SUITE(SpinLock) {
    STD_TEST(BasicLockUnlock) {
        SpinLock lock;

        lock.lock();
        lock.unlock();

        lock.lock();
        lock.unlock();
    }

    STD_TEST(ProtectsCounter) {
        SpinLock lock;
        int counter = 0;
        const int N = 4;
        const int ITERS = 10000;
        auto pool = ObjPool::fromMemory();
        auto* tp = ThreadPool::simple(pool.mutPtr(), N);

        for (int i = 0; i < N; ++i) {
            tp->submit([&]() {
                for (int j = 0; j < ITERS; ++j) {
                    lock.lock();
                    ++counter;
                    lock.unlock();
                }
            });
        }

        tp->join();
        STD_INSIST(counter == N * ITERS);
    }

    STD_TEST(InitiallyUnlocked) {
        SpinLock lock;

        STD_INSIST(lock.flag_ == 0);
        lock.lock();
        lock.unlock();
        STD_INSIST(lock.flag_ == 0);
    }
}
