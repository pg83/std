#include "poller.h"

#include <std/tst/ut.h>
#include <std/dbg/insist.h>
#include <std/sys/fd.h>
#include <std/ptr/scoped.h>

using namespace stl;

STD_TEST_SUITE(Poller) {
    STD_TEST(CreateDestroy) {
        ScopedPtr<PollerIface> p;
        p.ptr = PollerIface::create();
    }

    STD_TEST(WaitTimeout) {
        ScopedPtr<PollerIface> p;
        p.ptr = PollerIface::create();
        PollEvent ev[4];
        u32 n = p.ptr->wait(ev, 4, 1000);
        STD_INSIST(n == 0);
    }

    STD_TEST(ArmWritable) {
        ScopedPtr<PollerIface> p;
        p.ptr = PollerIface::create();
        ScopedFD r, w;
        createPipeFD(r, w);

        p.ptr->arm(w.get(), PollFlag::Out, (void*)0xBEEF);

        PollEvent ev[4];
        u32 n = p.ptr->wait(ev, 4, 100000);

        STD_INSIST(n == 1);
        STD_INSIST(ev[0].data == (void*)0xBEEF);
        STD_INSIST(ev[0].flags & PollFlag::Out);
    }

    STD_TEST(ArmReadable) {
        ScopedPtr<PollerIface> p;
        p.ptr = PollerIface::create();
        ScopedFD r, w;
        createPipeFD(r, w);

        p.ptr->arm(r.get(), PollFlag::In, (void*)0xCAFE);

        char c = 42;
        w.write(&c, 1);

        PollEvent ev[4];
        u32 n = p.ptr->wait(ev, 4, 100000);

        STD_INSIST(n == 1);
        STD_INSIST(ev[0].data == (void*)0xCAFE);
        STD_INSIST(ev[0].flags & PollFlag::In);
    }

    STD_TEST(DataPointer) {
        ScopedPtr<PollerIface> p;
        p.ptr = PollerIface::create();
        ScopedFD r, w;
        createPipeFD(r, w);

        void* sentinel = (void*)0x1234ABCD;
        p.ptr->arm(w.get(), PollFlag::Out, sentinel);

        PollEvent ev[4];
        u32 n = p.ptr->wait(ev, 4, 100000);

        STD_INSIST(n == 1);
        STD_INSIST(ev[0].data == sentinel);
    }

    STD_TEST(Disarm) {
        ScopedPtr<PollerIface> p;
        p.ptr = PollerIface::create();
        ScopedFD r, w;
        createPipeFD(r, w);

        p.ptr->arm(r.get(), PollFlag::In, nullptr);
        p.ptr->disarm(r.get());

        char c = 42;
        w.write(&c, 1);

        PollEvent ev[4];
        u32 n = p.ptr->wait(ev, 4, 1000);

        STD_INSIST(n == 0);
    }

    STD_TEST(OneshotSemantics) {
        ScopedPtr<PollerIface> p;
        p.ptr = PollerIface::create();
        ScopedFD r, w;
        createPipeFD(r, w);

        p.ptr->arm(r.get(), PollFlag::In, nullptr);

        char c = 42;
        w.write(&c, 1);

        PollEvent ev[4];
        u32 n = p.ptr->wait(ev, 4, 100000);
        STD_INSIST(n == 1);

        // after ONESHOT fires, second wait should time out
        n = p.ptr->wait(ev, 4, 1000);
        STD_INSIST(n == 0);
    }

    STD_TEST(RearmAfterOneshot) {
        ScopedPtr<PollerIface> p;
        p.ptr = PollerIface::create();
        ScopedFD r, w;
        createPipeFD(r, w);

        for (int i = 0; i < 3; ++i) {
            p.ptr->arm(r.get(), PollFlag::In, (void*)(uintptr_t)i);

            char c = (char)i;
            w.write(&c, 1);

            PollEvent ev[4];
            u32 n = p.ptr->wait(ev, 4, 100000);

            STD_INSIST(n == 1);
            STD_INSIST(ev[0].data == (void*)(uintptr_t)i);

            // drain
            r.read(&c, 1);
        }
    }

    STD_TEST(MultipleFds) {
        ScopedPtr<PollerIface> p;
        p.ptr = PollerIface::create();
        ScopedFD r1, w1, r2, w2;
        createPipeFD(r1, w1);
        createPipeFD(r2, w2);

        p.ptr->arm(w1.get(), PollFlag::Out, (void*)1);
        p.ptr->arm(w2.get(), PollFlag::Out, (void*)2);

        PollEvent ev[4];
        u32 n = p.ptr->wait(ev, 4, 100000);

        STD_INSIST(n == 2);
    }

    STD_TEST(DisarmNonExistent) {
        ScopedPtr<PollerIface> p;
        p.ptr = PollerIface::create();
        ScopedFD r, w;
        createPipeFD(r, w);

        // disarming an fd that was never armed should not crash
        p.ptr->disarm(r.get());
    }
}
