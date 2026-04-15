#include "parker.h"
#include "thread.h"
#include "runable.h"

#include <std/tst/ut.h>
#include <std/thr/poll_fd.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>

#include <poll.h>

using namespace stl;

STD_TEST_SUITE(Parker) {
    STD_TEST(SignalAlwaysWrites) {
        Parker p;
        p.signal();

        struct pollfd pfd;

        pfd.fd = p.fd();
        pfd.events = POLLIN;
        pfd.revents = 0;

        STD_INSIST(::poll(&pfd, 1, 0) == 1);
        STD_INSIST(pfd.revents & POLLIN);

        p.drain();
    }

    STD_TEST(UnparkWithoutParkIsNoop) {
        Parker p;
        p.unpark();

        struct pollfd pfd;

        pfd.fd = p.fd();
        pfd.events = POLLIN;
        pfd.revents = 0;

        STD_INSIST(::poll(&pfd, 1, 0) == 0);
    }

    STD_TEST(UnparkDuringParkWrites) {
        Parker p;
        bool written = false;

        p.park([&] {
            p.unpark();

            struct pollfd pfd;

            pfd.fd = p.fd();
            pfd.events = POLLIN;
            pfd.revents = 0;

            written = (::poll(&pfd, 1, 0) == 1);
        });

        STD_INSIST(written);
        p.drain();
    }

    STD_TEST(UnparkAfterParkLeaveIsNoop) {
        Parker p;

        p.park([&] {
        });

        p.unpark();

        struct pollfd pfd;

        pfd.fd = p.fd();
        pfd.events = POLLIN;
        pfd.revents = 0;

        STD_INSIST(::poll(&pfd, 1, 0) == 0);
    }

    STD_TEST(ParkFromAnotherThread) {
        Parker p;
        alignas(64) bool done = false;

        struct Worker: public Runable {
            Parker* p_;
            bool* done_;

            Worker(Parker* p, bool* d)
                : p_(p)
                , done_(d)
            {
            }

            void run() noexcept override {
                while (!stdAtomicFetch(done_, MemoryOrder::Acquire)) {
                    p_->unpark();
                }
            }
        };

        auto pool = ObjPool::fromMemory();
        Worker worker(&p, &done);
        auto& thread = *Thread::create(pool.mutPtr(), worker);

        for (int i = 0; i < 100; ++i) {
            p.park([&] {
                // spin until eventfd is readable or short timeout
                struct pollfd pfd;

                pfd.fd = p.fd();
                pfd.events = POLLIN;
                pfd.revents = 0;

                ::poll(&pfd, 1, 10);
            });

            p.drain();
        }

        stdAtomicStore(&done, true, MemoryOrder::Release);
        thread.join();
    }

    STD_TEST(MultipleUnparksCoalesce) {
        Parker p;

        p.park([&] {
            p.unpark();
            p.unpark();
            p.unpark();
        });

        struct pollfd pfd;

        pfd.fd = p.fd();
        pfd.events = POLLIN;
        pfd.revents = 0;

        STD_INSIST(::poll(&pfd, 1, 0) == 1);
        p.drain();
    }

    STD_TEST(SignalDuringParkWrites) {
        Parker p;

        p.park([&] {
            p.signal();
        });

        struct pollfd pfd;

        pfd.fd = p.fd();
        pfd.events = POLLIN;
        pfd.revents = 0;

        STD_INSIST(::poll(&pfd, 1, 0) == 1);
        p.drain();
    }
}
