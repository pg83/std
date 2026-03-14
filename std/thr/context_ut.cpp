#include "context.h"

#include <std/thr/runable.h>
#include <std/tst/ut.h>

#include <alloca.h>

using namespace stl;

namespace {
    static constexpr size_t kStackSize = 64 * 1024;

    static char gStack[kStackSize];

    struct SwitchState {
        Context* main;
        Context* worker;
        int counter = 0;
    };

    struct SwitchFn: public Runable {
        SwitchState* s;
        SwitchFn(SwitchState* s)
            : s(s)
        {
        }
        void run() noexcept override {
            ++s->counter;
            s->worker->switchTo(*s->main);
        }
    };

    struct MultiSwitchFn: public Runable {
        SwitchState* s;
        MultiSwitchFn(SwitchState* s)
            : s(s)
        {
        }
        void run() noexcept override {
            for (int i = 0; i < 5; ++i) {
                ++s->counter;
                s->worker->switchTo(*s->main);
            }
        }
    };
}

STD_TEST_SUITE(Context) {
    STD_TEST(SwitchBack) {
        auto* mainBuf = alloca(Context::implSize());
        auto* workerBuf = alloca(Context::implSize());
        SwitchState s;
        SwitchFn fn(&s);

        s.main = Context::create(mainBuf);
        s.worker = Context::create(workerBuf, gStack, kStackSize, fn);

        s.main->switchTo(*s.worker);

        STD_INSIST(s.counter == 1);
    }

    STD_TEST(MultiSwitch) {
        auto* mainBuf = alloca(Context::implSize());
        auto* workerBuf = alloca(Context::implSize());
        SwitchState s;
        MultiSwitchFn fn(&s);

        s.main = Context::create(mainBuf);
        s.worker = Context::create(workerBuf, gStack, kStackSize, fn);

        for (int i = 0; i < 5; ++i) {
            s.main->switchTo(*s.worker);
        }

        STD_INSIST(s.counter == 5);
    }

    STD_TEST(EhStatePreserved) {
        auto* mainBuf = alloca(Context::implSize());
        auto* workerBuf = alloca(Context::implSize());

        struct State {
            Context* main;
            Context* worker;
            int caught = 0;
        } s;

        struct EhFn: public Runable {
            State* s;
            EhFn(State* s)
                : s(s)
            {
            }
            void run() noexcept override {
                try {
                    throw 42;
                } catch (...) {
                    s->worker->switchTo(*s->main);
                    try {
                        throw;
                    } catch (int v) {
                        s->caught = v;
                    }
                }
                s->worker->switchTo(*s->main);
            }
        };

        static char stack[kStackSize];
        EhFn fn(&s);

        s.main = Context::create(mainBuf);
        s.worker = Context::create(workerBuf, stack, kStackSize, fn);

        s.main->switchTo(*s.worker);
        s.main->switchTo(*s.worker);

        STD_INSIST(s.caught == 42);
    }
}
