#include "ut.h"

#include <std/ios/sys.h>

#include <std/typ/support.h>

#include <std/mem/obj_pool.h>

#include <std/str/view.h>
#include <std/str/builder.h>

#include <std/alg/qsort.h>
#include <std/alg/range.h>

#include <std/dbg/color.h>
#include <std/dbg/panic.h>

#include <std/lib/vector.h>
#include <std/lib/singleton.h>

#include <stdio.h>

using namespace Std;

namespace {
    struct Exc {
    };

    struct Test {
        TestFunc* func;
        StringView fullName;

        inline Test(TestFunc* _func, StringView _fullName) noexcept
            : func(_func)
            , fullName(_fullName)
        {
        }

        inline void execute(OutBuf& outb) {
            try {
                func->execute();

                outb << Color::bright(AnsiColor::Green)
                     << StringView(u8"+ ") << fullName
                     << Color::reset() << endL;
            } catch (const Exc&) {
                outb << Color::bright(AnsiColor::Red)
                     << StringView(u8"- ") << fullName
                     << Color::reset() << endL;
            }
        }
    };

    struct GetOpt {
        bool listTests = false;
        Vector<StringView> includes;
    };

    struct Tests: public Vector<Test*> {
        Buffer str;
        ObjPool::Ref pool = ObjPool::fromMemory();
        Ctx* ctx = 0;
        OutBuf* outbuf = 0;

        inline void run(Ctx& ctx_) {
            ctx = &ctx_;

            quickSort(mutRange(*this), [](auto l, auto r) noexcept {
                return l->fullName < r->fullName;
            });

            auto old = setPanicHandler(panicHandler);
            execute(sysO);
            setPanicHandler(old);
        }

        inline void execute(OutBuf&& outb) {
            outbuf = &outb;

            for (auto test : range(*this)) {
                test->execute(outb);
            }

            outbuf = nullptr;
            outb.finish();
        }

        inline void handlePanic() {
            outbuf->flush();
            fflush(stdout);
            fflush(stderr);
            ctx->printTB();
            throw Exc();
        }

        inline void reg(TestFunc* func) {
            (StringBuilder(move(str)) << *func).xchg(str);
            pushBack(pool->make<Test>(func, pool->intern(str)));
            str.reset();
        }

        static inline auto& instance() noexcept {
            return singleton<Tests>();
        }

        static void panicHandler() {
            instance().handlePanic();
        }
    };
}

template <>
void Std::output<ZeroCopyOutput, TestFunc>(ZeroCopyOutput& buf, const TestFunc& test) {
    buf << test.suite() << StringView(u8"::") << test.name();
}

void Std::Ctx::run() {
    Tests::instance().run(*this);
}

void Std::registerTest(TestFunc* test) {
    Tests::instance().reg(test);
}
