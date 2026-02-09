#include "ut.h"

#include <std/ios/sys.h>
#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/alg/qsort.h>
#include <std/alg/range.h>
#include <std/dbg/color.h>
#include <std/dbg/panic.h>
#include <std/lib/vector.h>
#include <std/str/builder.h>
#include <std/typ/support.h>
#include <std/mem/obj_pool.h>
#include <std/lib/singleton.h>

#include <stdio.h>
#include <stdlib.h>

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

        inline bool execute(OutBuf& outb) {
            try {
                func->execute();

                outb << Color::bright(AnsiColor::Green)
                     << StringView(u8"+ ") << fullName
                     << Color::reset() << endL;
            } catch (const Exc&) {
                outb << Color::bright(AnsiColor::Red)
                     << StringView(u8"- ") << fullName
                     << Color::reset() << endL;

                return false;
            } catch (Exception& exc) {
                outb << Color::bright(AnsiColor::Red)
                     << exc.description() << endL
                     << StringView(u8"- ") << fullName
                     << Color::reset() << endL;

                return false;
            }

            return true;
        }
    };

    struct GetOpt {
        bool listTests = false;
        Vector<StringView> includes;

        inline GetOpt(Ctx& ctx) noexcept {
            for (int i = 1; i < ctx.argc; ++i) {
                includes.pushBack(StringView(ctx.argv[i]));
            }
        }

        inline bool matchesFilter(StringView testName) const noexcept {
            if (includes.empty()) {
                return true;
            }

            for (auto prefix : range(includes)) {
                if (testName.startsWith(prefix)) {
                    return true;
                }
            }

            return false;
        }
    };

    struct Tests: public Vector<Test*> {
        Buffer str;
        ObjPool::Ref pool = ObjPool::fromMemory();
        Ctx* ctx = 0;
        OutBuf* outbuf = 0;
        GetOpt* opt = 0;
        size_t ok = 0;
        size_t err = 0;
        size_t skip = 0;
        size_t mute = 0;

        inline void run(Ctx& ctx_) {
            ctx = &ctx_;
            opt = pool->make<GetOpt>(ctx_);

            quickSort(mutRange(*this), [](auto l, auto r) noexcept {
                return l->fullName < r->fullName;
            });

            auto old1 = setPanicHandler1(panicHandler1);
            auto old2 = setPanicHandler2(panicHandler2);

            execute(sysO);

            setPanicHandler1(old1);
            setPanicHandler2(old2);

            exit(err);
        }

        inline void execute(OutBuf&& outb) {
            outbuf = &outb;

            for (auto test : range(*this)) {
                if (test->fullName.search(u8"::_")) {
                    ++mute;
                } else if (!opt->matchesFilter(test->fullName)) {
                    ++skip;
                } else if (test->execute(outb)) {
                    ++ok;
                } else {
                    ++err;
                }
            }

            outb << Color::bright(AnsiColor::Green)
                 << StringView(u8"OK: ") << ok
                 << Color::reset();

            if (err) {
                outb << StringView(u8", ")
                     << Color::bright(AnsiColor::Red)
                     << StringView(u8"ERR: ") << err
                     << Color::reset();
            }

            if (skip) {
                outb << StringView(u8", ")
                     << Color::bright(AnsiColor::Yellow)
                     << StringView(u8"SKIP: ") << skip
                     << Color::reset();
            }

            if (mute) {
                outb << StringView(u8", ")
                     << Color::bright(AnsiColor::Blue)
                     << StringView(u8"MUTE: ") << mute
                     << Color::reset();
            }

            outb << endL << flsH;

            outbuf = nullptr;
            outb.finish();
        }

        inline void handlePanic1() {
            outbuf->flush();
        }

        inline void handlePanic2() {
            ctx->printTB();
            fflush(stdout);
            fflush(stderr);
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

        static void panicHandler1() {
            instance().handlePanic1();
        }

        static void panicHandler2() {
            instance().handlePanic2();
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
