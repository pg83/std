#include "ut.h"

#include <std/ios/sys.h>
#include <std/lib/list.h>
#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/dbg/color.h>
#include <std/dbg/panic.h>
#include <std/alg/range.h>
#include <std/lib/vector.h>
#include <std/str/builder.h>
#include <std/mem/obj_pool.h>
#include <std/lib/singleton.h>

#include <stdio.h>
#include <stdlib.h>

using namespace Std;

namespace {
    struct Exc {
    };

    static inline bool execute(TestFunc* func, ExecContext& ctx) {
        auto& outb = ctx.output();

        try {
            func->execute(ctx);

            outb << Color::bright(AnsiColor::Green)
                 << StringView(u8"+ ") << *func
                 << Color::reset() << endL;
        } catch (const Exc&) {
            outb << Color::bright(AnsiColor::Red)
                 << StringView(u8"- ") << *func
                 << Color::reset() << endL;

            return false;
        } catch (Exception& exc) {
            outb << Color::bright(AnsiColor::Red)
                 << exc.description() << endL
                 << StringView(u8"- ") << *func
                 << Color::reset() << endL;

            return false;
        }

        return true;
    }

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

            return matchesFilterStrong(testName);
        }

        inline bool matchesFilterStrong(StringView testName) const noexcept {
            for (auto prefix : range(includes)) {
                if (testName.startsWith(prefix)) {
                    return true;
                }
            }

            return false;
        }
    };

    struct Tests: public ExecContext, public IntrusiveList {
        ObjPool::Ref pool = ObjPool::fromMemory();
        Ctx* ctx = 0;
        OutBuf* outbuf = 0;
        GetOpt* opt = 0;
        size_t ok = 0;
        size_t err = 0;
        size_t skip = 0;
        size_t mute = 0;

        ZeroCopyOutput& output() const override {
            return *outbuf;
        }

        static inline bool compare(const TestFunc& l, const TestFunc& r) noexcept {
            return l.suite() < r.suite() || (l.suite() == r.suite() && l.name() < r.name());
        }

        inline void run(Ctx& ctx_) {
            ctx = &ctx_;
            opt = pool->make<GetOpt>(ctx_);

            sort([](const IntrusiveNode* l, const IntrusiveNode* r){
                return compare(*cvt(l), *cvt(r));
            });

            execute(sysO);
        }

        static inline TestFunc* cvt(const IntrusiveNode* node) noexcept {
            return (TestFunc*)((u8*)node - offsetof(TestFunc, node));
        }

        inline void execute(OutBuf&& outb) {
            outbuf = &outb;

            setPanicHandler1(panicHandler1);
            setPanicHandler2(panicHandler2);

            StringBuilder sb;

            while (!empty()) {
                auto test = cvt(popFront());

                sb.reset();
                sb << *test;

                if (test->name().startsWith(u8"_") && !opt->matchesFilterStrong(sb)) {
                    ++mute;
                } else if (!opt->matchesFilter(sb)) {
                    ++skip;
                } else if (::execute(test, *this)) {
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

            outb.finish();

            exit(err);
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
            pushBack(&func->node);
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
