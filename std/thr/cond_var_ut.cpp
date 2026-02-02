#include "cond_var.h"
#include "mutex.h"
#include "thread.h"

#include <std/tst/ut.h>

using namespace Std;

namespace {
    struct WaitSignalRunable: public Runable {
        Mutex* mutex;
        CondVar* cv;
        bool* ready;
        bool* executed;

        WaitSignalRunable(Mutex* m, CondVar* c, bool* r, bool* e)
            : mutex(m)
            , cv(c)
            , ready(r)
            , executed(e)
        {
        }

        void run() noexcept override {
            LockGuard lock(*mutex);
            while (!*ready) {
                cv->wait(*mutex);
            }
            *executed = true;
        }
    };

    struct WaitBroadcastRunable: public Runable {
        Mutex* mutex;
        CondVar* cv;
        bool* ready;
        int* counter;

        WaitBroadcastRunable(Mutex* m, CondVar* c, bool* r, int* cnt)
            : mutex(m)
            , cv(c)
            , ready(r)
            , counter(cnt)
        {
        }

        void run() noexcept override {
            LockGuard lock(*mutex);
            while (!*ready) {
                cv->wait(*mutex);
            }
            ++(*counter);
        }
    };

    struct ProducerConsumerRunable: public Runable {
        Mutex* mutex;
        CondVar* cv;
        int* data;
        bool* dataReady;
        int expectedValue;

        ProducerConsumerRunable(Mutex* m, CondVar* c, int* d, bool* dr, int expected)
            : mutex(m)
            , cv(c)
            , data(d)
            , dataReady(dr)
            , expectedValue(expected)
        {
        }

        void run() noexcept override {
            LockGuard lock(*mutex);
            while (!*dataReady) {
                cv->wait(*mutex);
            }
            STD_INSIST(*data == expectedValue);
        }
    };
}

STD_TEST_SUITE(CondVar) {
    STD_TEST(BasicConstruction) {
        CondVar cv1;
        CondVar cv2;
        CondVar cv3;
    }

    STD_TEST(SignalWithoutWaiters) {
        CondVar cv;
        cv.signal();
        cv.signal();
        cv.signal();
    }

    STD_TEST(BroadcastWithoutWaiters) {
        CondVar cv;
        cv.broadcast();
        cv.broadcast();
        cv.broadcast();
    }

    STD_TEST(WaitAndSignal) {
        Mutex mutex;
        CondVar cv;
        bool ready = false;
        bool executed = false;

        WaitSignalRunable runnable(&mutex, &cv, &ready, &executed);
        Thread thread(runnable);

        {
            LockGuard lock(mutex);
            ready = true;
            cv.signal();
        }

        thread.join();
        STD_INSIST(executed == true);
    }

    STD_TEST(WaitAndBroadcast) {
        Mutex mutex;
        CondVar cv;
        bool ready = false;
        bool executed = false;

        WaitSignalRunable runnable(&mutex, &cv, &ready, &executed);
        Thread thread(runnable);

        {
            LockGuard lock(mutex);
            ready = true;
            cv.broadcast();
        }

        thread.join();
        STD_INSIST(executed == true);
    }

    STD_TEST(MultipleWaitersBroadcast) {
        Mutex mutex;
        CondVar cv;
        bool ready = false;
        int counter = 0;

        WaitBroadcastRunable runnable1(&mutex, &cv, &ready, &counter);
        WaitBroadcastRunable runnable2(&mutex, &cv, &ready, &counter);
        WaitBroadcastRunable runnable3(&mutex, &cv, &ready, &counter);

        Thread thread1(runnable1);
        Thread thread2(runnable2);
        Thread thread3(runnable3);

        {
            LockGuard lock(mutex);
            ready = true;
            cv.broadcast();
        }

        thread1.join();
        thread2.join();
        thread3.join();

        STD_INSIST(counter == 3);
    }

    STD_TEST(SignalWakesOneWaiter) {
        Mutex mutex;
        CondVar cv;
        bool ready = false;
        int counter = 0;

        WaitBroadcastRunable runnable1(&mutex, &cv, &ready, &counter);
        WaitBroadcastRunable runnable2(&mutex, &cv, &ready, &counter);

        Thread thread1(runnable1);
        Thread thread2(runnable2);

        {
            LockGuard lock(mutex);
            ready = true;
            cv.signal();
            cv.signal();
        }

        thread1.join();
        thread2.join();

        STD_INSIST(counter == 2);
    }

    STD_TEST(ProducerConsumerPattern) {
        Mutex mutex;
        CondVar cv;
        int data = 0;
        bool dataReady = false;

        ProducerConsumerRunable consumer(&mutex, &cv, &data, &dataReady, 42);
        Thread consumerThread(consumer);

        {
            LockGuard lock(mutex);
            data = 42;
            dataReady = true;
            cv.signal();
        }

        consumerThread.join();
    }

    STD_TEST(MultipleProducersConsumers) {
        Mutex mutex;
        CondVar cv;
        int data = 0;
        bool dataReady = false;

        ProducerConsumerRunable consumer1(&mutex, &cv, &data, &dataReady, 100);
        ProducerConsumerRunable consumer2(&mutex, &cv, &data, &dataReady, 100);
        ProducerConsumerRunable consumer3(&mutex, &cv, &data, &dataReady, 100);

        Thread thread1(consumer1);
        Thread thread2(consumer2);
        Thread thread3(consumer3);

        {
            LockGuard lock(mutex);
            data = 100;
            dataReady = true;
            cv.broadcast();
        }

        thread1.join();
        thread2.join();
        thread3.join();
    }

    STD_TEST(SequentialWaitSignal) {
        Mutex mutex;
        CondVar cv;
        bool ready1 = false;
        bool ready2 = false;
        bool executed1 = false;
        bool executed2 = false;

        WaitSignalRunable runnable1(&mutex, &cv, &ready1, &executed1);
        Thread thread1(runnable1);

        {
            LockGuard lock(mutex);
            ready1 = true;
            cv.signal();
        }

        thread1.join();
        STD_INSIST(executed1 == true);

        WaitSignalRunable runnable2(&mutex, &cv, &ready2, &executed2);
        Thread thread2(runnable2);

        {
            LockGuard lock(mutex);
            ready2 = true;
            cv.signal();
        }

        thread2.join();
        STD_INSIST(executed2 == true);
    }

    STD_TEST(WaitWithSpuriousWakeup) {
        struct SpuriousWaitRunable: public Runable {
            Mutex* mutex;
            CondVar* cv;
            int* value;
            int targetValue;
            bool* done;

            SpuriousWaitRunable(Mutex* m, CondVar* c, int* v, int target, bool* d)
                : mutex(m)
                , cv(c)
                , value(v)
                , targetValue(target)
                , done(d)
            {
            }

            void run() noexcept override {
                LockGuard lock(*mutex);
                while (*value != targetValue) {
                    cv->wait(*mutex);
                }
                *done = true;
            }
        };

        Mutex mutex;
        CondVar cv;
        int value = 0;
        bool done = false;

        SpuriousWaitRunable runnable(&mutex, &cv, &value, 5, &done);
        Thread thread(runnable);

        for (int i = 1; i <= 5; ++i) {
            LockGuard lock(mutex);
            value = i;
            cv.signal();
        }

        thread.join();
        STD_INSIST(done == true);
        STD_INSIST(value == 5);
    }

    STD_TEST(MultipleCondVarsOneMutex) {
        Mutex mutex;
        CondVar cv1;
        CondVar cv2;
        bool ready1 = false;
        bool ready2 = false;
        bool executed1 = false;
        bool executed2 = false;

        WaitSignalRunable runnable1(&mutex, &cv1, &ready1, &executed1);
        WaitSignalRunable runnable2(&mutex, &cv2, &ready2, &executed2);

        Thread thread1(runnable1);
        Thread thread2(runnable2);

        {
            LockGuard lock(mutex);
            ready1 = true;
            cv1.signal();
        }

        {
            LockGuard lock(mutex);
            ready2 = true;
            cv2.signal();
        }

        thread1.join();
        thread2.join();

        STD_INSIST(executed1 == true);
        STD_INSIST(executed2 == true);
    }

    STD_TEST(BroadcastMultipleTimes) {
        Mutex mutex;
        CondVar cv;
        bool ready = false;
        int counter = 0;

        WaitBroadcastRunable runnable1(&mutex, &cv, &ready, &counter);
        WaitBroadcastRunable runnable2(&mutex, &cv, &ready, &counter);

        Thread thread1(runnable1);
        Thread thread2(runnable2);

        {
            LockGuard lock(mutex);
            ready = true;
            cv.broadcast();
            cv.broadcast();
            cv.broadcast();
        }

        thread1.join();
        thread2.join();

        STD_INSIST(counter == 2);
    }

    STD_TEST(WaitWithPredicatePattern) {
        struct PredicateWaitRunable: public Runable {
            Mutex* mutex;
            CondVar* cv;
            bool* predicate;
            int* result;

            PredicateWaitRunable(Mutex* m, CondVar* c, bool* p, int* r)
                : mutex(m)
                , cv(c)
                , predicate(p)
                , result(r)
            {
            }

            void run() noexcept override {
                LockGuard lock(*mutex);
                while (!*predicate) {
                    cv->wait(*mutex);
                }
                *result = 123;
            }
        };

        Mutex mutex;
        CondVar cv;
        bool predicate = false;
        int result = 0;

        PredicateWaitRunable runnable(&mutex, &cv, &predicate, &result);
        Thread thread(runnable);

        {
            LockGuard lock(mutex);
            predicate = true;
            cv.signal();
        }

        thread.join();
        STD_INSIST(result == 123);
    }

    STD_TEST(DetachWithCondVar) {
        struct DetachWaitRunable: public Runable {
            Mutex* mutex;
            CondVar* cv;
            bool* ready;
            bool* done;

            DetachWaitRunable(Mutex* m, CondVar* c, bool* r, bool* d)
                : mutex(m)
                , cv(c)
                , ready(r)
                , done(d)
            {
            }

            void run() noexcept override {
                LockGuard lock(*mutex);
                while (!*ready) {
                    cv->wait(*mutex);
                }
                *done = true;
                cv->signal();
            }
        };

        Mutex mutex;
        CondVar cv;
        bool ready = false;
        bool done = false;

        {
            LockGuard lock(mutex);
            DetachWaitRunable runnable(&mutex, &cv, &ready, &done);
            detach(runnable);

            ready = true;
            cv.signal();

            while (!done) {
                cv.wait(mutex);
            }
        }

        STD_INSIST(done == true);
    }

    STD_TEST(ComplexSynchronizationPattern) {
        struct Phase1Runable: public Runable {
            Mutex* mutex;
            CondVar* cv;
            int* phase;

            Phase1Runable(Mutex* m, CondVar* c, int* p)
                : mutex(m)
                , cv(c)
                , phase(p)
            {
            }

            void run() noexcept override {
                LockGuard lock(*mutex);
                while (*phase != 1) {
                    cv->wait(*mutex);
                }
                *phase = 2;
                cv->broadcast();
            }
        };

        struct Phase2Runable: public Runable {
            Mutex* mutex;
            CondVar* cv;
            int* phase;

            Phase2Runable(Mutex* m, CondVar* c, int* p)
                : mutex(m)
                , cv(c)
                , phase(p)
            {
            }

            void run() noexcept override {
                LockGuard lock(*mutex);
                while (*phase != 2) {
                    cv->wait(*mutex);
                }
                *phase = 3;
                cv->broadcast();
            }
        };

        Mutex mutex;
        CondVar cv;
        int phase = 0;

        Phase1Runable runnable1(&mutex, &cv, &phase);
        Phase2Runable runnable2(&mutex, &cv, &phase);

        Thread thread1(runnable1);
        Thread thread2(runnable2);

        {
            LockGuard lock(mutex);
            phase = 1;
            cv.broadcast();

            while (phase != 3) {
                cv.wait(mutex);
            }
        }

        thread1.join();
        thread2.join();

        STD_INSIST(phase == 3);
    }

    STD_TEST(RapidSignaling) {
        Mutex mutex;
        CondVar cv;

        for (int i = 0; i < 1000; ++i) {
            LockGuard lock(mutex);
            cv.signal();
        }
    }

    STD_TEST(RapidBroadcasting) {
        Mutex mutex;
        CondVar cv;

        for (int i = 0; i < 1000; ++i) {
            LockGuard lock(mutex);
            cv.broadcast();
        }
    }

    STD_TEST(ManyWaitersBroadcast) {
        Mutex mutex;
        CondVar cv;
        bool ready = false;
        int counter = 0;
        const int numThreads = 10;

        WaitBroadcastRunable runnable1(&mutex, &cv, &ready, &counter);
        WaitBroadcastRunable runnable2(&mutex, &cv, &ready, &counter);
        WaitBroadcastRunable runnable3(&mutex, &cv, &ready, &counter);
        WaitBroadcastRunable runnable4(&mutex, &cv, &ready, &counter);
        WaitBroadcastRunable runnable5(&mutex, &cv, &ready, &counter);
        WaitBroadcastRunable runnable6(&mutex, &cv, &ready, &counter);
        WaitBroadcastRunable runnable7(&mutex, &cv, &ready, &counter);
        WaitBroadcastRunable runnable8(&mutex, &cv, &ready, &counter);
        WaitBroadcastRunable runnable9(&mutex, &cv, &ready, &counter);
        WaitBroadcastRunable runnable10(&mutex, &cv, &ready, &counter);

        Thread thread1(runnable1);
        Thread thread2(runnable2);
        Thread thread3(runnable3);
        Thread thread4(runnable4);
        Thread thread5(runnable5);
        Thread thread6(runnable6);
        Thread thread7(runnable7);
        Thread thread8(runnable8);
        Thread thread9(runnable9);
        Thread thread10(runnable10);

        {
            LockGuard lock(mutex);
            ready = true;
            cv.broadcast();
        }

        thread1.join();
        thread2.join();
        thread3.join();
        thread4.join();
        thread5.join();
        thread6.join();
        thread7.join();
        thread8.join();
        thread9.join();
        thread10.join();

        STD_INSIST(counter == numThreads);
    }
}
