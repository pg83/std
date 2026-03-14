#include "context.h"

#include <std/tst/ut.h>

using namespace stl;

namespace {
    static constexpr size_t kStackSize = 64 * 1024;

    static char gStack[kStackSize];

    struct SwitchState {
        Context* main;
        Context* worker;
        int counter = 0;
    };

    static void switchFn(u32 lo, u32 hi) {
        auto* s = (SwitchState*)(((uintptr_t)hi << 32) | lo);
        ++s->counter;
        s->worker->switchTo(*s->main);
    }

    static void multiSwitchFn(u32 lo, u32 hi) {
        auto* s = (SwitchState*)(((uintptr_t)hi << 32) | lo);

        for (int i = 0; i < 5; ++i) {
            ++s->counter;
            s->worker->switchTo(*s->main);
        }
    }
}

STD_TEST_SUITE(Context) {
    STD_TEST(SwitchBack) {
        alignas(max_align_t) char mainBuf[Context::kBufSize];
        alignas(max_align_t) char workerBuf[Context::kBufSize];
        SwitchState s;

        s.main = Context::create(mainBuf);
        s.worker = Context::create(workerBuf, gStack, kStackSize, switchFn, (uintptr_t)&s);

        s.main->switchTo(*s.worker);

        STD_INSIST(s.counter == 1);
    }

    STD_TEST(MultiSwitch) {
        alignas(max_align_t) char mainBuf[Context::kBufSize];
        alignas(max_align_t) char workerBuf[Context::kBufSize];
        SwitchState s;

        s.main = Context::create(mainBuf);
        s.worker = Context::create(workerBuf, gStack, kStackSize, multiSwitchFn, (uintptr_t)&s);

        for (int i = 0; i < 5; ++i) {
            s.main->switchTo(*s.worker);
        }

        STD_INSIST(s.counter == 5);
    }

    STD_TEST(EhStatePreserved) {
        // Exception state (caughtExceptions) must survive a context switch:
        // throw in worker, switch back to main mid-catch, switch back, rethrow.
        alignas(max_align_t) char mainBuf[Context::kBufSize];
        alignas(max_align_t) char workerBuf[Context::kBufSize];

        struct State {
            Context* main;
            Context* worker;
            int caught = 0;
        } s;

        static char stack[kStackSize];

        s.main = Context::create(mainBuf);
        s.worker = Context::create(workerBuf, stack, kStackSize, [](u32 lo, u32 hi) {
            auto* s = (State*)(((uintptr_t)hi << 32) | lo);

            try {
                throw 42;
            } catch (...) {
                s->worker->switchTo(*s->main); // yield mid-catch
                try {
                    throw;
                } catch (int v) {
                    s->caught = v;
                }
            }

            s->worker->switchTo(*s->main);
        }, (uintptr_t)&s);

        s.main->switchTo(*s.worker); // enter worker, stops mid-catch
        s.main->switchTo(*s.worker); // resume, rethrow

        STD_INSIST(s.caught == 42);
    }
}
