#include "thread.h"
#include "mutex.h"
#include "cond_var.h"

#include <std/tst/ut.h>

using namespace Std;

namespace {
    struct CounterRunable: public Runable {
        int* counter;

        explicit CounterRunable(int* c)
            : counter(c)
        {
        }

        void run() noexcept override {
            ++(*counter);
        }
    };

    struct MultiIncrementRunable: public Runable {
        int* counter;
        int iterations;

        MultiIncrementRunable(int* c, int iter)
            : counter(c)
            , iterations(iter)
        {
        }

        void run() noexcept override {
            for (int i = 0; i < iterations; ++i) {
                ++(*counter);
            }
        }
    };
}

STD_TEST_SUITE(Thread) {
    STD_TEST(BasicThreadCreationAndJoin) {
        int counter = 0;
        CounterRunable runnable(&counter);

        Thread thread(runnable);
        thread.join();

        STD_INSIST(counter == 1);
    }

    STD_TEST(MultipleThreads) {
        int counter1 = 0;
        int counter2 = 0;
        int counter3 = 0;

        CounterRunable runnable1(&counter1);
        CounterRunable runnable2(&counter2);
        CounterRunable runnable3(&counter3);

        Thread thread1(runnable1);
        Thread thread2(runnable2);
        Thread thread3(runnable3);

        thread1.join();
        thread2.join();
        thread3.join();

        STD_INSIST(counter1 == 1);
        STD_INSIST(counter2 == 1);
        STD_INSIST(counter3 == 1);
    }

    STD_TEST(ThreadWithMultipleIncrements) {
        int counter = 0;
        MultiIncrementRunable runnable(&counter, 1000);

        Thread thread(runnable);
        thread.join();

        STD_INSIST(counter == 1000);
    }

    STD_TEST(SequentialThreadCreation) {
        for (int i = 0; i < 10; ++i) {
            int counter = 0;
            CounterRunable runnable(&counter);

            Thread thread(runnable);
            thread.join();

            STD_INSIST(counter == 1);
        }
    }

    STD_TEST(MultipleThreadsWithDifferentWorkloads) {
        int counter1 = 0;
        int counter2 = 0;
        int counter3 = 0;

        MultiIncrementRunable runnable1(&counter1, 100);
        MultiIncrementRunable runnable2(&counter2, 500);
        MultiIncrementRunable runnable3(&counter3, 1000);

        Thread thread1(runnable1);
        Thread thread2(runnable2);
        Thread thread3(runnable3);

        thread1.join();
        thread2.join();
        thread3.join();

        STD_INSIST(counter1 == 100);
        STD_INSIST(counter2 == 500);
        STD_INSIST(counter3 == 1000);
    }

    STD_TEST(ThreadJoinOrder) {
        int counter1 = 0;
        int counter2 = 0;

        CounterRunable runnable1(&counter1);
        CounterRunable runnable2(&counter2);

        Thread thread1(runnable1);
        Thread thread2(runnable2);

        thread2.join();
        thread1.join();

        STD_INSIST(counter1 == 1);
        STD_INSIST(counter2 == 1);
    }

    STD_TEST(EmptyRunableExecution) {
        struct EmptyRunable: public Runable {
            bool* executed;

            explicit EmptyRunable(bool* e)
                : executed(e)
            {
            }

            void run() noexcept override {
                *executed = true;
            }
        };

        bool executed = false;
        EmptyRunable runnable(&executed);

        Thread thread(runnable);
        thread.join();

        STD_INSIST(executed == true);
    }

    STD_TEST(DetachBasicExecution) {
        struct DetachRunable: public Runable {
            Mutex* mutex;
            CondVar* cv;
            bool* executed;

            DetachRunable(Mutex* m, CondVar* c, bool* e)
                : mutex(m)
                , cv(c)
                , executed(e)
            {
            }

            void run() noexcept override {
                LockGuard lock(*mutex);
                *executed = true;
                cv->signal();
            }
        };

        Mutex mutex;
        CondVar cv;
        bool executed = false;

        {
            LockGuard lock(mutex);
            DetachRunable runnable(&mutex, &cv, &executed);
            detach(runnable);

            while (!executed) {
                cv.wait(mutex);
            }
        }

        STD_INSIST(executed == true);
    }

    STD_TEST(DetachWithCounter) {
        struct DetachCounterRunable: public Runable {
            Mutex* mutex;
            CondVar* cv;
            int* counter;

            DetachCounterRunable(Mutex* m, CondVar* c, int* cnt)
                : mutex(m)
                , cv(c)
                , counter(cnt)
            {
            }

            void run() noexcept override {
                LockGuard lock(*mutex);
                ++(*counter);
                cv->signal();
            }
        };

        Mutex mutex;
        CondVar cv;
        int counter = 0;

        {
            LockGuard lock(mutex);
            DetachCounterRunable runnable(&mutex, &cv, &counter);
            detach(runnable);

            while (counter == 0) {
                cv.wait(mutex);
            }
        }

        STD_INSIST(counter == 1);
    }

    STD_TEST(DetachMultipleThreads) {
        struct DetachMultiRunable: public Runable {
            Mutex* mutex;
            CondVar* cv;
            int* counter;
            int* completed;

            DetachMultiRunable(Mutex* m, CondVar* c, int* cnt, int* cmp)
                : mutex(m)
                , cv(c)
                , counter(cnt)
                , completed(cmp)
            {
            }

            void run() noexcept override {
                LockGuard lock(*mutex);
                ++(*counter);
                ++(*completed);
                cv->broadcast();
            }
        };

        Mutex mutex;
        CondVar cv;
        int counter = 0;
        int completed = 0;
        const int numThreads = 5;

        DetachMultiRunable runnable1(&mutex, &cv, &counter, &completed);
        DetachMultiRunable runnable2(&mutex, &cv, &counter, &completed);
        DetachMultiRunable runnable3(&mutex, &cv, &counter, &completed);
        DetachMultiRunable runnable4(&mutex, &cv, &counter, &completed);
        DetachMultiRunable runnable5(&mutex, &cv, &counter, &completed);

        {
            LockGuard lock(mutex);
            detach(runnable1);
            detach(runnable2);
            detach(runnable3);
            detach(runnable4);
            detach(runnable5);

            while (completed < numThreads) {
                cv.wait(mutex);
            }
        }

        STD_INSIST(counter == numThreads);
        STD_INSIST(completed == numThreads);
    }

    STD_TEST(DetachWithMultipleIncrements) {
        struct DetachMultiIncrRunable: public Runable {
            Mutex* mutex;
            CondVar* cv;
            int* counter;
            int iterations;
            bool* done;

            DetachMultiIncrRunable(Mutex* m, CondVar* c, int* cnt, int iter, bool* d)
                : mutex(m)
                , cv(c)
                , counter(cnt)
                , iterations(iter)
                , done(d)
            {
            }

            void run() noexcept override {
                for (int i = 0; i < iterations; ++i) {
                    LockGuard lock(*mutex);
                    ++(*counter);
                }

                {
                    LockGuard lock(*mutex);
                    *done = true;
                    cv->signal();
                }
            }
        };

        Mutex mutex;
        CondVar cv;
        int counter = 0;
        bool done = false;
        const int iterations = 100;

        {
            LockGuard lock(mutex);
            DetachMultiIncrRunable runnable(&mutex, &cv, &counter, iterations, &done);
            detach(runnable);

            while (!done) {
                cv.wait(mutex);
            }
        }

        STD_INSIST(counter == iterations);
        STD_INSIST(done == true);
    }

    STD_TEST(DetachSequential) {
        struct DetachSeqRunable: public Runable {
            Mutex* mutex;
            CondVar* cv;
            int* value;
            int expected;

            DetachSeqRunable(Mutex* m, CondVar* c, int* v, int exp)
                : mutex(m)
                , cv(c)
                , value(v)
                , expected(exp)
            {
            }

            void run() noexcept override {
                LockGuard lock(*mutex);
                while (*value != expected) {
                    cv->wait(*mutex);
                }
                ++(*value);
                cv->broadcast();
            }
        };

        Mutex mutex;
        CondVar cv;
        int value = 0;

        DetachSeqRunable runnable1(&mutex, &cv, &value, 0);
        DetachSeqRunable runnable2(&mutex, &cv, &value, 1);
        DetachSeqRunable runnable3(&mutex, &cv, &value, 2);

        detach(runnable1);
        detach(runnable2);
        detach(runnable3);

        {
            LockGuard lock(mutex);
            while (value < 3) {
                cv.wait(mutex);
            }
        }

        STD_INSIST(value == 3);
    }

    STD_TEST(DetachWithSharedResource) {
        struct DetachSharedRunable: public Runable {
            Mutex* mutex;
            CondVar* cv;
            int* resource;
            int myValue;
            bool* done;

            DetachSharedRunable(Mutex* m, CondVar* c, int* r, int val, bool* d)
                : mutex(m)
                , cv(c)
                , resource(r)
                , myValue(val)
                , done(d)
            {
            }

            void run() noexcept override {
                {
                    LockGuard lock(*mutex);
                    *resource += myValue;
                }

                {
                    LockGuard lock(*mutex);
                    *done = true;
                    cv->signal();
                }
            }
        };

        Mutex mutex;
        CondVar cv;
        int resource = 0;
        bool done1 = false;
        bool done2 = false;

        DetachSharedRunable runnable1(&mutex, &cv, &resource, 10, &done1);
        DetachSharedRunable runnable2(&mutex, &cv, &resource, 20, &done2);

        detach(runnable1);
        detach(runnable2);

        {
            LockGuard lock(mutex);
            while (!done1 || !done2) {
                cv.wait(mutex);
            }
        }

        STD_INSIST(resource == 30);
        STD_INSIST(done1 == true);
        STD_INSIST(done2 == true);
    }
}
