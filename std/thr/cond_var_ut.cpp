#include "coro.h"
#include "pool.h"
#include "mutex.h"
#include "guard.h"
#include "thread.h"
#include "runable.h"
#include "cond_var.h"
#include "wait_group.h"

#include <std/tst/ut.h>
#include <std/mem/obj_pool.h>

using namespace stl;

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
            LockGuard lock(mutex);
            while (!*ready) {
                cv->wait(mutex);
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
            LockGuard lock(mutex);
            while (!*ready) {
                cv->wait(mutex);
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
            LockGuard lock(mutex);
            while (!*dataReady) {
                cv->wait(mutex);
            }
            STD_INSIST(*data == expectedValue);
        }
    };
}

STD_TEST_SUITE(CondVar) {
    STD_TEST(BasicConstruction) {
        auto pool = ObjPool::fromMemory();

        CondVar::create(pool.mutPtr());
        CondVar::create(pool.mutPtr());
        CondVar::create(pool.mutPtr());
    }

    STD_TEST(SignalWithoutWaiters) {
        auto pool = ObjPool::fromMemory();
        auto cv = CondVar::create(pool.mutPtr());

        cv->signal();
        cv->signal();
        cv->signal();
    }

    STD_TEST(BroadcastWithoutWaiters) {
        auto pool = ObjPool::fromMemory();
        auto cv = CondVar::create(pool.mutPtr());

        cv->broadcast();
        cv->broadcast();
        cv->broadcast();
    }

    STD_TEST(WaitAndSignal) {
        auto pool = ObjPool::fromMemory();
        auto& mutex = *Mutex::create(pool.mutPtr());
        auto cv = CondVar::create(pool.mutPtr());
        bool ready = false;
        bool executed = false;

        WaitSignalRunable runnable(&mutex, cv, &ready, &executed);
        auto& thread = *Thread::create(pool.mutPtr(), runnable);

        {
            LockGuard lock(&mutex);
            ready = true;
            cv->signal();
        }

        thread.join();
        STD_INSIST(executed == true);
    }

    STD_TEST(WaitAndBroadcast) {
        auto pool = ObjPool::fromMemory();
        auto& mutex = *Mutex::create(pool.mutPtr());
        auto cv = CondVar::create(pool.mutPtr());
        bool ready = false;
        bool executed = false;

        WaitSignalRunable runnable(&mutex, cv, &ready, &executed);
        auto& thread = *Thread::create(pool.mutPtr(), runnable);

        {
            LockGuard lock(&mutex);
            ready = true;
            cv->broadcast();
        }

        thread.join();
        STD_INSIST(executed == true);
    }

    STD_TEST(MultipleWaitersBroadcast) {
        auto pool = ObjPool::fromMemory();
        auto& mutex = *Mutex::create(pool.mutPtr());
        auto cv = CondVar::create(pool.mutPtr());
        bool ready = false;
        int counter = 0;

        WaitBroadcastRunable runnable1(&mutex, cv, &ready, &counter);
        WaitBroadcastRunable runnable2(&mutex, cv, &ready, &counter);
        WaitBroadcastRunable runnable3(&mutex, cv, &ready, &counter);

        auto& thread1 = *Thread::create(pool.mutPtr(), runnable1);
        auto& thread2 = *Thread::create(pool.mutPtr(), runnable2);
        auto& thread3 = *Thread::create(pool.mutPtr(), runnable3);

        {
            LockGuard lock(&mutex);
            ready = true;
            cv->broadcast();
        }

        thread1.join();
        thread2.join();
        thread3.join();

        STD_INSIST(counter == 3);
    }

    STD_TEST(SignalWakesOneWaiter) {
        auto pool = ObjPool::fromMemory();
        auto& mutex = *Mutex::create(pool.mutPtr());
        auto cv = CondVar::create(pool.mutPtr());
        bool ready = false;
        int counter = 0;

        WaitBroadcastRunable runnable1(&mutex, cv, &ready, &counter);
        WaitBroadcastRunable runnable2(&mutex, cv, &ready, &counter);

        auto& thread1 = *Thread::create(pool.mutPtr(), runnable1);
        auto& thread2 = *Thread::create(pool.mutPtr(), runnable2);

        {
            LockGuard lock(&mutex);
            ready = true;
            cv->signal();
            cv->signal();
        }

        thread1.join();
        thread2.join();

        STD_INSIST(counter == 2);
    }

    STD_TEST(ProducerConsumerPattern) {
        auto pool = ObjPool::fromMemory();
        auto& mutex = *Mutex::create(pool.mutPtr());
        auto cv = CondVar::create(pool.mutPtr());
        int data = 0;
        bool dataReady = false;

        ProducerConsumerRunable consumer(&mutex, cv, &data, &dataReady, 42);
        auto& consumerThread = *Thread::create(pool.mutPtr(), consumer);

        {
            LockGuard lock(&mutex);
            data = 42;
            dataReady = true;
            cv->signal();
        }

        consumerThread.join();
    }

    STD_TEST(MultipleProducersConsumers) {
        auto pool = ObjPool::fromMemory();
        auto& mutex = *Mutex::create(pool.mutPtr());
        auto cv = CondVar::create(pool.mutPtr());
        int data = 0;
        bool dataReady = false;

        ProducerConsumerRunable consumer1(&mutex, cv, &data, &dataReady, 100);
        ProducerConsumerRunable consumer2(&mutex, cv, &data, &dataReady, 100);
        ProducerConsumerRunable consumer3(&mutex, cv, &data, &dataReady, 100);

        auto& thread1 = *Thread::create(pool.mutPtr(), consumer1);
        auto& thread2 = *Thread::create(pool.mutPtr(), consumer2);
        auto& thread3 = *Thread::create(pool.mutPtr(), consumer3);

        {
            LockGuard lock(&mutex);
            data = 100;
            dataReady = true;
            cv->broadcast();
        }

        thread1.join();
        thread2.join();
        thread3.join();
    }

    STD_TEST(SequentialWaitSignal) {
        auto pool = ObjPool::fromMemory();
        auto& mutex = *Mutex::create(pool.mutPtr());
        auto cv = CondVar::create(pool.mutPtr());
        bool ready1 = false;
        bool ready2 = false;
        bool executed1 = false;
        bool executed2 = false;

        WaitSignalRunable runnable1(&mutex, cv, &ready1, &executed1);
        auto& thread1 = *Thread::create(pool.mutPtr(), runnable1);

        {
            LockGuard lock(&mutex);
            ready1 = true;
            cv->signal();
        }

        thread1.join();
        STD_INSIST(executed1 == true);

        WaitSignalRunable runnable2(&mutex, cv, &ready2, &executed2);
        auto& thread2 = *Thread::create(pool.mutPtr(), runnable2);

        {
            LockGuard lock(&mutex);
            ready2 = true;
            cv->signal();
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
                LockGuard lock(mutex);
                while (*value != targetValue) {
                    cv->wait(mutex);
                }
                *done = true;
            }
        };

        auto pool = ObjPool::fromMemory();
        auto& mutex = *Mutex::create(pool.mutPtr());
        auto cv = CondVar::create(pool.mutPtr());
        int value = 0;
        bool done = false;

        SpuriousWaitRunable runnable(&mutex, cv, &value, 5, &done);
        auto& thread = *Thread::create(pool.mutPtr(), runnable);

        for (int i = 1; i <= 5; ++i) {
            LockGuard lock(&mutex);
            value = i;
            cv->signal();
        }

        thread.join();
        STD_INSIST(done == true);
        STD_INSIST(value == 5);
    }

    STD_TEST(MultipleCondVarsOneMutex) {
        auto pool = ObjPool::fromMemory();
        auto& mutex = *Mutex::create(pool.mutPtr());
        auto cv1 = CondVar::create(pool.mutPtr());
        auto cv2 = CondVar::create(pool.mutPtr());
        bool ready1 = false;
        bool ready2 = false;
        bool executed1 = false;
        bool executed2 = false;

        WaitSignalRunable runnable1(&mutex, cv1, &ready1, &executed1);
        WaitSignalRunable runnable2(&mutex, cv2, &ready2, &executed2);

        auto& thread1 = *Thread::create(pool.mutPtr(), runnable1);
        auto& thread2 = *Thread::create(pool.mutPtr(), runnable2);

        {
            LockGuard lock(&mutex);
            ready1 = true;
            cv1->signal();
        }

        {
            LockGuard lock(&mutex);
            ready2 = true;
            cv2->signal();
        }

        thread1.join();
        thread2.join();

        STD_INSIST(executed1 == true);
        STD_INSIST(executed2 == true);
    }

    STD_TEST(BroadcastMultipleTimes) {
        auto pool = ObjPool::fromMemory();
        auto& mutex = *Mutex::create(pool.mutPtr());
        auto cv = CondVar::create(pool.mutPtr());
        bool ready = false;
        int counter = 0;

        WaitBroadcastRunable runnable1(&mutex, cv, &ready, &counter);
        WaitBroadcastRunable runnable2(&mutex, cv, &ready, &counter);

        auto& thread1 = *Thread::create(pool.mutPtr(), runnable1);
        auto& thread2 = *Thread::create(pool.mutPtr(), runnable2);

        {
            LockGuard lock(&mutex);
            ready = true;
            cv->broadcast();
            cv->broadcast();
            cv->broadcast();
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
                LockGuard lock(mutex);
                while (!*predicate) {
                    cv->wait(mutex);
                }
                *result = 123;
            }
        };

        auto pool = ObjPool::fromMemory();
        auto& mutex = *Mutex::create(pool.mutPtr());
        auto cv = CondVar::create(pool.mutPtr());
        bool predicate = false;
        int result = 0;

        PredicateWaitRunable runnable(&mutex, cv, &predicate, &result);
        auto& thread = *Thread::create(pool.mutPtr(), runnable);

        {
            LockGuard lock(&mutex);
            predicate = true;
            cv->signal();
        }

        thread.join();
        STD_INSIST(result == 123);
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
                LockGuard lock(mutex);
                while (*phase != 1) {
                    cv->wait(mutex);
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
                LockGuard lock(mutex);
                while (*phase != 2) {
                    cv->wait(mutex);
                }
                *phase = 3;
                cv->broadcast();
            }
        };

        auto pool = ObjPool::fromMemory();
        auto& mutex = *Mutex::create(pool.mutPtr());
        auto cv = CondVar::create(pool.mutPtr());
        int phase = 0;

        Phase1Runable runnable1(&mutex, cv, &phase);
        Phase2Runable runnable2(&mutex, cv, &phase);

        auto& thread1 = *Thread::create(pool.mutPtr(), runnable1);
        auto& thread2 = *Thread::create(pool.mutPtr(), runnable2);

        {
            LockGuard lock(&mutex);
            phase = 1;
            cv->broadcast();

            while (phase != 3) {
                cv->wait(&mutex);
            }
        }

        thread1.join();
        thread2.join();

        STD_INSIST(phase == 3);
    }

    STD_TEST(RapidSignaling) {
        auto pool = ObjPool::fromMemory();
        auto& mutex = *Mutex::create(pool.mutPtr());
        auto cv = CondVar::create(pool.mutPtr());

        for (int i = 0; i < 1000; ++i) {
            LockGuard lock(&mutex);
            cv->signal();
        }
    }

    STD_TEST(RapidBroadcasting) {
        auto pool = ObjPool::fromMemory();
        auto& mutex = *Mutex::create(pool.mutPtr());
        auto cv = CondVar::create(pool.mutPtr());

        for (int i = 0; i < 1000; ++i) {
            LockGuard lock(&mutex);
            cv->broadcast();
        }
    }

    STD_TEST(ManyWaitersBroadcast) {
        auto pool = ObjPool::fromMemory();
        auto& mutex = *Mutex::create(pool.mutPtr());
        auto cv = CondVar::create(pool.mutPtr());
        bool ready = false;
        int counter = 0;
        const int numThreads = 10;

        WaitBroadcastRunable runnable1(&mutex, cv, &ready, &counter);
        WaitBroadcastRunable runnable2(&mutex, cv, &ready, &counter);
        WaitBroadcastRunable runnable3(&mutex, cv, &ready, &counter);
        WaitBroadcastRunable runnable4(&mutex, cv, &ready, &counter);
        WaitBroadcastRunable runnable5(&mutex, cv, &ready, &counter);
        WaitBroadcastRunable runnable6(&mutex, cv, &ready, &counter);
        WaitBroadcastRunable runnable7(&mutex, cv, &ready, &counter);
        WaitBroadcastRunable runnable8(&mutex, cv, &ready, &counter);
        WaitBroadcastRunable runnable9(&mutex, cv, &ready, &counter);
        WaitBroadcastRunable runnable10(&mutex, cv, &ready, &counter);

        auto& thread1 = *Thread::create(pool.mutPtr(), runnable1);
        auto& thread2 = *Thread::create(pool.mutPtr(), runnable2);
        auto& thread3 = *Thread::create(pool.mutPtr(), runnable3);
        auto& thread4 = *Thread::create(pool.mutPtr(), runnable4);
        auto& thread5 = *Thread::create(pool.mutPtr(), runnable5);
        auto& thread6 = *Thread::create(pool.mutPtr(), runnable6);
        auto& thread7 = *Thread::create(pool.mutPtr(), runnable7);
        auto& thread8 = *Thread::create(pool.mutPtr(), runnable8);
        auto& thread9 = *Thread::create(pool.mutPtr(), runnable9);
        auto& thread10 = *Thread::create(pool.mutPtr(), runnable10);

        {
            LockGuard lock(&mutex);
            ready = true;
            cv->broadcast();
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

STD_TEST_SUITE(CoroCondVar) {
    STD_TEST(SignalWithoutWaiters) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto cv = exec->createCondVar(pool.mutPtr());

        exec->spawn([&] {
            cv->signal();
            cv->signal();
            cv->signal();
        });

        exec->join();
    }

    STD_TEST(BroadcastWithoutWaiters) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto cv = exec->createCondVar(pool.mutPtr());

        exec->spawn([&] {
            cv->broadcast();
            cv->broadcast();
            cv->broadcast();
        });

        exec->join();
    }

    STD_TEST(WaitAndSignal) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        bool ready = false;
        bool executed = false;
        auto& mtx = *Mutex::create(pool.mutPtr(), exec);
        auto cv = exec->createCondVar(pool.mutPtr());

        exec->spawn([&] {
            LockGuard lock(&mtx);
            while (!ready) {
                cv->wait(&mtx);
            }
            executed = true;
        });

        exec->spawn([&] {
            LockGuard lock(&mtx);
            ready = true;
            cv->signal();
        });

        exec->join();
        STD_INSIST(executed == true);
    }

    STD_TEST(WaitAndBroadcast) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        bool ready = false;
        bool executed = false;
        auto& mtx = *Mutex::create(pool.mutPtr(), exec);
        auto cv = exec->createCondVar(pool.mutPtr());

        exec->spawn([&] {
            LockGuard lock(&mtx);
            while (!ready) {
                cv->wait(&mtx);
            }
            executed = true;
        });

        exec->spawn([&] {
            LockGuard lock(&mtx);
            ready = true;
            cv->broadcast();
        });

        exec->join();
        STD_INSIST(executed == true);
    }

    STD_TEST(MultipleWaitersBroadcast) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        const int N = 3;
        bool ready = false;
        int counter = 0;
        auto& mtx = *Mutex::create(pool.mutPtr(), exec);
        auto cv = exec->createCondVar(pool.mutPtr());

        for (int i = 0; i < N; ++i) {
            exec->spawn([&] {
                LockGuard lock(&mtx);
                while (!ready) {
                    cv->wait(&mtx);
                }
                ++counter;
            });
        }

        exec->spawn([&] {
            LockGuard lock(&mtx);
            ready = true;
            cv->broadcast();
        });

        exec->join();
        STD_INSIST(counter == N);
    }

    STD_TEST(SignalWakesOneWaiter) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        bool ready = false;
        int counter = 0;
        auto& mtx = *Mutex::create(pool.mutPtr(), exec);
        auto cv = exec->createCondVar(pool.mutPtr());

        for (int i = 0; i < 2; ++i) {
            exec->spawn([&] {
                LockGuard lock(&mtx);
                while (!ready) {
                    cv->wait(&mtx);
                }
                ++counter;
            });
        }

        exec->spawn([&] {
            LockGuard lock(&mtx);
            ready = true;
            cv->signal();
            cv->signal();
        });

        exec->join();
        STD_INSIST(counter == 2);
    }

    STD_TEST(ProducerConsumerPattern) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int data = 0;
        bool dataReady = false;
        auto& mtx = *Mutex::create(pool.mutPtr(), exec);
        auto cv = exec->createCondVar(pool.mutPtr());

        exec->spawn([&] {
            LockGuard lock(&mtx);
            while (!dataReady) {
                cv->wait(&mtx);
            }
            STD_INSIST(data == 42);
        });

        exec->spawn([&] {
            LockGuard lock(&mtx);
            data = 42;
            dataReady = true;
            cv->signal();
        });

        exec->join();
    }

    STD_TEST(MultipleProducersConsumers) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        const int N = 3;
        int data = 0;
        bool dataReady = false;
        auto& mtx = *Mutex::create(pool.mutPtr(), exec);
        auto cv = exec->createCondVar(pool.mutPtr());

        for (int i = 0; i < N; ++i) {
            exec->spawn([&] {
                LockGuard lock(&mtx);
                while (!dataReady) {
                    cv->wait(&mtx);
                }
                STD_INSIST(data == 100);
            });
        }

        exec->spawn([&] {
            LockGuard lock(&mtx);
            data = 100;
            dataReady = true;
            cv->broadcast();
        });

        exec->join();
    }

    STD_TEST(SequentialWaitSignal) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        bool ready1 = false, ready2 = false;
        bool executed1 = false, executed2 = false;
        auto& mtx = *Mutex::create(pool.mutPtr(), exec);
        auto cv = exec->createCondVar(pool.mutPtr());

        {
            auto& done = *WaitGroup::create(pool.mutPtr(), 1);
            exec->spawn([&] {
                LockGuard lock(&mtx);
                while (!ready1) {
                    cv->wait(&mtx);
                }
                executed1 = true;
                done.done();
            });
            exec->spawn([&] {
                LockGuard lock(&mtx);
                ready1 = true;
                cv->signal();
            });
            done.wait();
        }

        STD_INSIST(executed1 == true);

        {
            auto& done = *WaitGroup::create(pool.mutPtr(), 1);
            exec->spawn([&] {
                LockGuard lock(&mtx);
                while (!ready2) {
                    cv->wait(&mtx);
                }
                executed2 = true;
                done.done();
            });
            exec->spawn([&] {
                LockGuard lock(&mtx);
                ready2 = true;
                cv->signal();
            });
            done.wait();
        }

        exec->join();
        STD_INSIST(executed2 == true);
    }

    STD_TEST(WaitWithSpuriousWakeup) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int value = 0;
        auto& mtx = *Mutex::create(pool.mutPtr(), exec);
        auto cv = exec->createCondVar(pool.mutPtr());

        exec->spawn([&] {
            LockGuard lock(&mtx);
            while (value != 5) {
                cv->wait(&mtx);
            }
        });

        exec->spawn([&] {
            for (int i = 1; i <= 5; ++i) {
                LockGuard lock(&mtx);
                value = i;
                cv->signal();
            }
        });

        exec->join();
        STD_INSIST(value == 5);
    }

    STD_TEST(MultipleCondVarsOneMutex) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        bool ready1 = false, ready2 = false;
        bool executed1 = false, executed2 = false;
        auto& mtx = *Mutex::create(pool.mutPtr(), exec);
        auto cv1 = exec->createCondVar(pool.mutPtr());
        auto cv2 = exec->createCondVar(pool.mutPtr());

        exec->spawn([&] {
            LockGuard lock(&mtx);
            while (!ready1) {
                cv1->wait(&mtx);
            }
            executed1 = true;
        });

        exec->spawn([&] {
            LockGuard lock(&mtx);
            while (!ready2) {
                cv2->wait(&mtx);
            }
            executed2 = true;
        });

        exec->spawn([&] {
            LockGuard lock(&mtx);
            ready1 = true;
            cv1->signal();
        });

        exec->spawn([&] {
            LockGuard lock(&mtx);
            ready2 = true;
            cv2->signal();
        });

        exec->join();
        STD_INSIST(executed1 == true);
        STD_INSIST(executed2 == true);
    }

    STD_TEST(BroadcastMultipleTimes) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        bool ready = false;
        int counter = 0;
        auto& mtx = *Mutex::create(pool.mutPtr(), exec);
        auto cv = exec->createCondVar(pool.mutPtr());

        for (int i = 0; i < 2; ++i) {
            exec->spawn([&] {
                LockGuard lock(&mtx);
                while (!ready) {
                    cv->wait(&mtx);
                }
                ++counter;
            });
        }

        exec->spawn([&] {
            LockGuard lock(&mtx);
            ready = true;
            cv->broadcast();
            cv->broadcast();
            cv->broadcast();
        });

        exec->join();
        STD_INSIST(counter == 2);
    }

    STD_TEST(WaitWithPredicatePattern) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        bool predicate = false;
        int result = 0;
        auto& mtx = *Mutex::create(pool.mutPtr(), exec);
        auto cv = exec->createCondVar(pool.mutPtr());

        exec->spawn([&] {
            LockGuard lock(&mtx);
            while (!predicate) {
                cv->wait(&mtx);
            }
            result = 123;
        });

        exec->spawn([&] {
            LockGuard lock(&mtx);
            predicate = true;
            cv->signal();
        });

        exec->join();
        STD_INSIST(result == 123);
    }

    STD_TEST(ComplexSynchronizationPattern) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        int phase = 0;
        auto& mtx = *Mutex::create(pool.mutPtr(), exec);
        auto cv = exec->createCondVar(pool.mutPtr());

        exec->spawn([&] {
            LockGuard lock(&mtx);
            while (phase != 1) {
                cv->wait(&mtx);
            }
            phase = 2;
            cv->broadcast();
        });

        exec->spawn([&] {
            LockGuard lock(&mtx);
            while (phase != 2) {
                cv->wait(&mtx);
            }
            phase = 3;
            cv->broadcast();
        });

        exec->spawn([&] {
            LockGuard lock(&mtx);
            phase = 1;
            cv->broadcast();
            while (phase != 3) {
                cv->wait(&mtx);
            }
        });

        exec->join();
        STD_INSIST(phase == 3);
    }

    STD_TEST(RapidSignaling) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto& mtx = *Mutex::create(pool.mutPtr(), exec);
        auto cv = exec->createCondVar(pool.mutPtr());

        exec->spawn([&] {
            for (int i = 0; i < 1000; ++i) {
                LockGuard lock(&mtx);
                cv->signal();
            }
        });

        exec->join();
    }

    STD_TEST(RapidBroadcasting) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        auto& mtx = *Mutex::create(pool.mutPtr(), exec);
        auto cv = exec->createCondVar(pool.mutPtr());

        exec->spawn([&] {
            for (int i = 0; i < 1000; ++i) {
                LockGuard lock(&mtx);
                cv->broadcast();
            }
        });

        exec->join();
    }

    STD_TEST(ManyWaitersBroadcast) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);
        const int N = 10;
        bool ready = false;
        int counter = 0;
        auto& mtx = *Mutex::create(pool.mutPtr(), exec);
        auto cv = exec->createCondVar(pool.mutPtr());

        for (int i = 0; i < N; ++i) {
            exec->spawn([&] {
                LockGuard lock(&mtx);
                while (!ready) {
                    cv->wait(&mtx);
                }
                ++counter;
            });
        }

        exec->spawn([&] {
            LockGuard lock(&mtx);
            ready = true;
            cv->broadcast();
        });

        exec->join();
        STD_INSIST(counter == N);
    }
}
