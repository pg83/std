#include "coro.h"
#include "pool.h"
#include "mutex.h"
#include "guard.h"

#include <std/tst/ut.h>
#include <std/mem/obj_pool.h>

using namespace stl;

STD_TEST_SUITE(Mutex) {
    STD_TEST(BasicLockUnlock) {
        Mutex mutex;

        mutex.lock();
        mutex.unlock();

        mutex.lock();
        mutex.unlock();
    }

    STD_TEST(TryLockSuccess) {
        Mutex mutex;

        bool locked = mutex.tryLock();
        STD_INSIST(locked == true);

        mutex.unlock();
    }

    STD_TEST(TryLockFail) {
        Mutex mutex;

        mutex.lock();
        bool locked = mutex.tryLock();
        STD_INSIST(locked == false);

        mutex.unlock();
    }

    STD_TEST(MultipleLockUnlock) {
        Mutex mutex;

        for (int i = 0; i < 100; ++i) {
            mutex.lock();
            mutex.unlock();
        }
    }

    STD_TEST(TryLockMultiple) {
        Mutex mutex;

        for (int i = 0; i < 100; ++i) {
            bool locked = mutex.tryLock();
            STD_INSIST(locked == true);
            mutex.unlock();
        }
    }

    STD_TEST(LockGuardBasic) {
        Mutex mutex;

        {
            LockGuard guard(mutex);
        }

        bool locked = mutex.tryLock();
        STD_INSIST(locked == true);
        mutex.unlock();
    }

    STD_TEST(LockGuardNested) {
        Mutex mutex1;
        Mutex mutex2;

        {
            LockGuard guard1(mutex1);
            {
                LockGuard guard2(mutex2);
            }
        }

        bool locked1 = mutex1.tryLock();
        bool locked2 = mutex2.tryLock();

        STD_INSIST(locked1 == true);
        STD_INSIST(locked2 == true);

        mutex1.unlock();
        mutex2.unlock();
    }

    STD_TEST(LockGuardException) {
        Mutex mutex;

        try {
            LockGuard guard(mutex);
            throw 42;
        } catch (int) {
        }

        bool locked = mutex.tryLock();
        STD_INSIST(locked == true);
        mutex.unlock();
    }

    STD_TEST(MutexConstruction) {
        Mutex mutex1;
        Mutex mutex2;
        Mutex mutex3;

        mutex1.lock();
        mutex2.lock();
        mutex3.lock();

        mutex1.unlock();
        mutex2.unlock();
        mutex3.unlock();
    }

    STD_TEST(LockUnlockSequence) {
        Mutex mutex;

        mutex.lock();
        mutex.unlock();

        bool locked = mutex.tryLock();
        STD_INSIST(locked == true);
        mutex.unlock();

        mutex.lock();
        mutex.unlock();
    }
}

STD_TEST_SUITE(SpinMutex) {
    STD_TEST(BasicLockUnlock) {
        Mutex mtx(Mutex::spinLock(nullptr));

        mtx.lock();
        mtx.unlock();

        mtx.lock();
        mtx.unlock();
    }

    STD_TEST(TryLockSuccess) {
        Mutex mtx(Mutex::spinLock(nullptr));

        bool locked = mtx.tryLock();
        STD_INSIST(locked == true);

        mtx.unlock();
    }

    STD_TEST(TryLockFail) {
        Mutex mtx(Mutex::spinLock(nullptr));

        mtx.lock();
        bool locked = mtx.tryLock();
        STD_INSIST(locked == false);

        mtx.unlock();
    }

    STD_TEST(MultipleLockUnlock) {
        Mutex mtx(Mutex::spinLock(nullptr));

        for (int i = 0; i < 100; ++i) {
            mtx.lock();
            mtx.unlock();
        }
    }

    STD_TEST(LockGuardBasic) {
        Mutex mtx(Mutex::spinLock(nullptr));

        {
            LockGuard guard(mtx);
        }

        bool locked = mtx.tryLock();
        STD_INSIST(locked == true);
        mtx.unlock();
    }

    STD_TEST(Contention) {
        Mutex mtx(Mutex::spinLock(nullptr));
        auto opool = ObjPool::fromMemory();
        auto pool = ThreadPool::simple(opool.mutPtr(), 4);
        int counter = 0;

        for (int i = 0; i < 4; ++i) {
            pool->submit([&] {
                for (int j = 0; j < 1000; ++j) {
                    LockGuard guard(mtx);
                    ++counter;
                }
            });
        }

        pool->join();
        STD_INSIST(counter == 4000);
    }

    STD_TEST(CoroBasicLockUnlock) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        Mutex mtx(Mutex::spinLock(exec));

        exec->spawn([&] {
            mtx.lock();
            mtx.unlock();

            mtx.lock();
            mtx.unlock();
        });

        exec->join();
    }

    STD_TEST(CoroContention) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        Mutex mtx(Mutex::spinLock(exec));
        int counter = 0;

        for (int i = 0; i < 4; ++i) {
            exec->spawn([&] {
                for (int j = 0; j < 1000; ++j) {
                    LockGuard guard(mtx);
                    ++counter;
                }
            });
        }

        exec->join();
        STD_INSIST(counter == 4000);
    }
}

STD_TEST_SUITE(CoroMutex) {
    STD_TEST(BasicLockUnlock) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        Mutex mtx(exec);

        exec->spawn([&] {
            mtx.lock();
            mtx.unlock();

            mtx.lock();
            mtx.unlock();
        });

        exec->join();
    }

    STD_TEST(TryLockSuccess) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        bool result = false;
        Mutex mtx(exec);

        exec->spawn([&] {
            result = mtx.tryLock();
            if (result) {
                mtx.unlock();
            }
        });

        exec->join();
        STD_INSIST(result == true);
    }

    STD_TEST(TryLockFail) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        bool result = true;
        Mutex mtx(exec);

        exec->spawn([&] {
            mtx.lock();
            result = mtx.tryLock();
            mtx.unlock();
        });

        exec->join();
        STD_INSIST(result == false);
    }

    STD_TEST(MultipleLockUnlock) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        Mutex mtx(exec);

        exec->spawn([&] {
            for (int i = 0; i < 100; ++i) {
                mtx.lock();
                mtx.unlock();
            }
        });

        exec->join();
    }

    STD_TEST(TryLockMultiple) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        bool ok = true;
        Mutex mtx(exec);

        exec->spawn([&] {
            for (int i = 0; i < 100; ++i) {
                if (!mtx.tryLock()) {
                    ok = false;
                    break;
                }
                mtx.unlock();
            }
        });

        exec->join();
        STD_INSIST(ok == true);
    }

    STD_TEST(LockGuardBasic) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        bool result = false;
        Mutex mtx(exec);

        exec->spawn([&] {
            {
                LockGuard guard(mtx);
            }
            result = mtx.tryLock();
            if (result) {
                mtx.unlock();
            }
        });

        exec->join();
        STD_INSIST(result == true);
    }

    STD_TEST(LockGuardNested) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        bool r1 = false, r2 = false;
        Mutex mtx1(exec);
        Mutex mtx2(exec);

        exec->spawn([&] {
            {
                LockGuard g1(mtx1);
                {
                    LockGuard g2(mtx2);
                }
            }
            r1 = mtx1.tryLock();
            r2 = mtx2.tryLock();
            if (r1) {
                mtx1.unlock();
            }
            if (r2) {
                mtx2.unlock();
            }
        });

        exec->join();
        STD_INSIST(r1 == true);
        STD_INSIST(r2 == true);
    }

    STD_TEST(MutexConstruction) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        Mutex mtx1(exec);
        Mutex mtx2(exec);
        Mutex mtx3(exec);

        exec->spawn([&] {
            mtx1.lock();
            mtx2.lock();
            mtx3.lock();
            mtx1.unlock();
            mtx2.unlock();
            mtx3.unlock();
        });

        exec->join();
    }

    STD_TEST(LockUnlockSequence) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        bool result = false;
        Mutex mtx(exec);

        exec->spawn([&] {
            mtx.lock();
            mtx.unlock();

            result = mtx.tryLock();
            if (result) {
                mtx.unlock();
            }

            mtx.lock();
            mtx.unlock();
        });

        exec->join();
        STD_INSIST(result == true);
    }
}
