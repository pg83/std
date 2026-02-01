#include "mutex.h"

#include <std/tst/ut.h>

using namespace Std;

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

        bool locked = mutex.try_lock();
        STD_INSIST(locked == true);

        mutex.unlock();
    }

    STD_TEST(TryLockFail) {
        Mutex mutex;

        mutex.lock();
        bool locked = mutex.try_lock();
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
            bool locked = mutex.try_lock();
            STD_INSIST(locked == true);
            mutex.unlock();
        }
    }

    STD_TEST(LockGuardBasic) {
        Mutex mutex;

        {
            LockGuard guard(mutex);
        }

        bool locked = mutex.try_lock();
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

        bool locked1 = mutex1.try_lock();
        bool locked2 = mutex2.try_lock();

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

        bool locked = mutex.try_lock();
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

        bool locked = mutex.try_lock();
        STD_INSIST(locked == true);
        mutex.unlock();

        mutex.lock();
        mutex.unlock();
    }
}
