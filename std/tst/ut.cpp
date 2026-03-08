#include "ut.h"
#include "ctx.h"

#include <std/ios/sys.h>
#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/dbg/color.h>
#include <std/dbg/panic.h>
#include <std/alg/range.h>
#include <std/map/treap.h>
#include <std/lib/vector.h>
#include <std/str/builder.h>

#include <stdio.h>
#include <stdlib.h>

using namespace stl;

namespace {
    struct Exc {
    };

    static bool execute(TestFunc* func, ExecContext& ctx) {
        auto& outb = ctx.output();

        try {
            func->execute(ctx);

            outb << Color::bright(AnsiColor::Green)
                 << StringView(u8"+ ")
                 << *func
                 << Color::reset()
                 << endL;
        } catch (const Exc&) {
            outb << Color::bright(AnsiColor::Red)
                 << StringView(u8"- ")
                 << *func
                 << Color::reset()
                 << endL;

            return false;
        } catch (Exception& exc) {
            outb << Color::bright(AnsiColor::Red)
                 << exc.description()
                 << endL
                 << StringView(u8"- ")
                 << *func
                 << Color::reset()
                 << endL;

            return false;
        }

        return true;
    }

    struct GetOpt {
        Vector<StringView> includes;
        Vector<StringView> excludes;

        GetOpt(Ctx& ctx) {
            for (int i = 1; i < ctx.argc; ++i) {
                StringView arg(ctx.argv[i]);

                if (arg.startsWith(u8"-") && arg.length() > 1) {
                    excludes.pushBack(StringView(arg.data() + 1, arg.length() - 1));
                } else {
                    includes.pushBack(arg);
                }
            }
        }

        bool matchesFilter(StringView testName) const {
            if (matchesExclude(testName)) {
                return false;
            }

            if (includes.empty()) {
                return true;
            }

            return matchesFilterStrong(testName);
        }

        bool matchesFilterStrong(StringView testName) const {
            if (matchesExclude(testName)) {
                return false;
            }

            for (auto prefix : range(includes)) {
                if (testName.startsWith(prefix)) {
                    return true;
                }
            }

            return false;
        }

        bool matchesExclude(StringView testName) const {
            for (auto prefix : range(excludes)) {
                if (testName.startsWith(prefix)) {
                    return true;
                }
            }

            return false;
        }
    };

    struct Tests: public ExecContext, public Treap {
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

        bool cmp(void* l, void* r) const override {
            return compare(*(const TestFunc*)(l), *(const TestFunc*)(r));
        }

        static bool compare(const TestFunc& l, const TestFunc& r) {
            return l.suite() < r.suite() || (l.suite() == r.suite() && l.name() < r.name());
        }

        void run(Ctx& ctx_) {
            ctx = &ctx_;
            opt = new GetOpt(ctx_);
            execute(sysO);
        }

        void execute(OutBuf&& outb) {
            outbuf = &outb;

            setPanicHandler1(panicHandler1);
            setPanicHandler2(panicHandler2);

            StringBuilder sb;

            visit([&](void* el) {
                auto test = (TestFunc*)el;

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

                // outb.flush();
            });

            outb << Color::bright(AnsiColor::Green)
                 << StringView(u8"OK: ")
                 << ok
                 << Color::reset();

            if (err) {
                outb << StringView(u8", ")
                     << Color::bright(AnsiColor::Red)
                     << StringView(u8"ERR: ")
                     << err
                     << Color::reset();
            }

            if (skip) {
                outb << StringView(u8", ")
                     << Color::bright(AnsiColor::Yellow)
                     << StringView(u8"SKIP: ")
                     << skip
                     << Color::reset();
            }

            if (mute) {
                outb << StringView(u8", ")
                     << Color::bright(AnsiColor::Blue)
                     << StringView(u8"MUTE: ")
                     << mute
                     << Color::reset();
            }

            outb << endL << flsH << finI;

            exit(err);
        }

        void handlePanic1() {
            outbuf->flush();
        }

        void handlePanic2() {
            ctx->printTB();
            fflush(stdout);
            fflush(stderr);
            throw Exc();
        }

        static auto& instance() {
            static auto res = new Tests();

            return *res;
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
void stl::output<ZeroCopyOutput, TestFunc>(ZeroCopyOutput& buf, const TestFunc& test) {
    buf << test.suite()
        << StringView(u8"::")
        << test.name();
}

void Ctx::run() {
    Tests::instance().run(*this);
}

void TestFunc::registerMe() {
    Tests::instance().insert(this);
}
