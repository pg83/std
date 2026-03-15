#include "poller.h"

#include <std/tst/ut.h>
#include <std/dbg/insist.h>
#include <std/sys/fd.h>
#include <std/mem/obj_pool.h>

using namespace stl;

STD_TEST_SUITE(Poller) {
    STD_TEST(CreateDestroy) {
        auto pool = ObjPool::fromMemory();
        auto* p = PollerIface::create(pool.mutPtr());
        (void)p;
    }

    STD_TEST(WaitTimeout) {
        auto pool = ObjPool::fromMemory();
        auto* p = PollerIface::create(pool.mutPtr());
        u32 n = 0;
        p->wait([&](PollEvent*) { ++n; }, 1000);
        STD_INSIST(n == 0);
    }

    STD_TEST(ArmWritable) {
        auto pool = ObjPool::fromMemory();
        auto* p = PollerIface::create(pool.mutPtr());
        ScopedFD r, w;
        createPipeFD(r, w);

        p->arm(w.get(), PollFlag::Out, (void*)0xBEEF);

        u32 n = 0;
        p->wait([&](PollEvent* ev) {
            ++n;
            STD_INSIST(ev->data == (void*)0xBEEF);
            STD_INSIST(ev->flags & PollFlag::Out);
        }, 100000);

        STD_INSIST(n == 1);
    }

    STD_TEST(ArmReadable) {
        auto pool = ObjPool::fromMemory();
        auto* p = PollerIface::create(pool.mutPtr());
        ScopedFD r, w;
        createPipeFD(r, w);

        p->arm(r.get(), PollFlag::In, (void*)0xCAFE);

        char c = 42;
        w.write(&c, 1);

        u32 n = 0;
        p->wait([&](PollEvent* ev) {
            ++n;
            STD_INSIST(ev->data == (void*)0xCAFE);
            STD_INSIST(ev->flags & PollFlag::In);
        }, 100000);

        STD_INSIST(n == 1);
    }

    STD_TEST(DataPointer) {
        auto pool = ObjPool::fromMemory();
        auto* p = PollerIface::create(pool.mutPtr());
        ScopedFD r, w;
        createPipeFD(r, w);

        void* sentinel = (void*)0x1234ABCD;
        p->arm(w.get(), PollFlag::Out, sentinel);

        u32 n = 0;
        p->wait([&](PollEvent* ev) {
            ++n;
            STD_INSIST(ev->data == sentinel);
        }, 100000);

        STD_INSIST(n == 1);
    }

    STD_TEST(Disarm) {
        auto pool = ObjPool::fromMemory();
        auto* p = PollerIface::create(pool.mutPtr());
        ScopedFD r, w;
        createPipeFD(r, w);

        p->arm(r.get(), PollFlag::In, nullptr);
        p->disarm(r.get());

        char c = 42;
        w.write(&c, 1);

        u32 n = 0;
        p->wait([&](PollEvent*) { ++n; }, 1000);

        STD_INSIST(n == 0);
    }

    STD_TEST(OneshotSemantics) {
        auto pool = ObjPool::fromMemory();
        auto* p = PollerIface::create(pool.mutPtr());
        ScopedFD r, w;
        createPipeFD(r, w);

        p->arm(r.get(), PollFlag::In, nullptr);

        char c = 42;
        w.write(&c, 1);

        u32 n = 0;
        p->wait([&](PollEvent*) { ++n; }, 100000);
        STD_INSIST(n == 1);

        // after ONESHOT fires, second wait should time out
        n = 0;
        p->wait([&](PollEvent*) { ++n; }, 1000);
        STD_INSIST(n == 0);
    }

    STD_TEST(RearmAfterOneshot) {
        auto pool = ObjPool::fromMemory();
        auto* p = PollerIface::create(pool.mutPtr());
        ScopedFD r, w;
        createPipeFD(r, w);

        for (int i = 0; i < 3; ++i) {
            p->arm(r.get(), PollFlag::In, (void*)(uintptr_t)i);

            char c = (char)i;
            w.write(&c, 1);

            u32 n = 0;
            p->wait([&](PollEvent* ev) {
                ++n;
                STD_INSIST(ev->data == (void*)(uintptr_t)i);
            }, 100000);

            STD_INSIST(n == 1);

            // drain
            r.read(&c, 1);
        }
    }

    STD_TEST(MultipleFds) {
        auto pool = ObjPool::fromMemory();
        auto* p = PollerIface::create(pool.mutPtr());
        ScopedFD r1, w1, r2, w2;
        createPipeFD(r1, w1);
        createPipeFD(r2, w2);

        p->arm(w1.get(), PollFlag::Out, (void*)1);
        p->arm(w2.get(), PollFlag::Out, (void*)2);

        u32 n = 0;
        p->wait([&](PollEvent*) { ++n; }, 100000);

        STD_INSIST(n == 2);
    }

    STD_TEST(DisarmNonExistent) {
        auto pool = ObjPool::fromMemory();
        auto* p = PollerIface::create(pool.mutPtr());
        ScopedFD r, w;
        createPipeFD(r, w);

        // disarming an fd that was never armed should not crash
        p->disarm(r.get());
    }
}
