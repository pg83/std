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
#include <std/sym/s_map.h>
#include <std/thr/pool.h>
#include <std/thr/mutex.h>
#include <std/mem/obj_pool.h>

#include <stdio.h>
#include <stdlib.h>

using namespace stl;

namespace {
    struct Exc {
    };

    struct BufferedExecContext: public ExecContext {
        mutable StringBuilder buf_;

        ZeroCopyOutput& output() const override {
            return buf_;
        }
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
        SymbolMap<StringView> opts;

        GetOpt(Ctx& ctx) noexcept;
        void help() const noexcept;
        bool matchesFilter(StringView testName) const noexcept;
        bool matchesFilterStrong(StringView testName) const noexcept;
        bool matchesExclude(StringView testName) const noexcept;

        size_t threads() const noexcept {
            if (auto* sv = opts.find(StringView(u8"threads")); sv) {
                return (size_t)sv->stou();
            }

            return 0;
        }
    };

    struct Tests: public Treap {
        Ctx* ctx = 0;
        OutBuf* outbuf = 0;
        GetOpt* opt = 0;
        size_t ok = 0;
        size_t err = 0;
        size_t skip = 0;
        size_t mute = 0;

        bool cmp(void* l, void* r) const noexcept override {
            return compare(*(const TestFunc*)(l), *(const TestFunc*)(r));
        }

        static bool compare(const TestFunc& l, const TestFunc& r) noexcept {
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

            auto opool = ObjPool::fromMemory();
            auto pool = ThreadPool::simple(opool.mutPtr(), opt->threads());

            Mutex mutex;

            visit([&](void* el) {
                auto test = (TestFunc*)el;

                pool->submit([&, test] {
                    StringBuilder sb;
                    sb << *test;

                    LockGuard lock(mutex);

                    if (test->name().startsWith(u8"_") && !opt->matchesFilterStrong(StringView(sb))) {
                        ++mute;
                    } else if (!opt->matchesFilter(StringView(sb))) {
                        ++skip;
                    } else {
                        BufferedExecContext bctx;
                        bool ok_ = UnlockGuard(mutex).run([&] { return ::execute(test, bctx); });
                        outb << StringView(bctx.buf_);
                        if (ok_) { ++ok; } else { ++err; }
                    }
                });
            });

            pool->join();

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

        static auto& instance() noexcept {
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

GetOpt::GetOpt(Ctx& ctx) noexcept {
    for (int i = 1; i < ctx.argc; ++i) {
        StringView arg(ctx.argv[i]);

        if (arg.startsWith(u8"--") && arg.length() > 2) {
            StringView key(arg.data() + 2, arg.length() - 2);
            StringView value(u8"1");

            if (auto* eq = key.memChr('='); eq) {
                value = StringView(eq + 1, key.end());
                key = StringView(key.begin(), eq);
            }

            opts[key] = value;
        } else if (arg.startsWith(u8"-") && arg.length() > 1) {
            excludes.pushBack(StringView(arg.data() + 1, arg.length() - 1));
        } else {
            includes.pushBack(arg);
        }
    }

    help();
}

void GetOpt::help() const noexcept {
    if (!opts.find(StringView(u8"help"))) {
        return;
    }

    auto out = sysE;

    out << StringView(u8"Usage: test-binary [FILTER...] [--OPTION[=VALUE]]") << endL
        << endL
        << StringView(u8"Filters:") << endL
        << StringView(u8"  Suite::Test    run tests whose full name starts with the prefix") << endL
        << StringView(u8"  -Suite::Test   exclude tests matching the prefix") << endL
        << StringView(u8"  (tests prefixed with _ are muted unless explicitly included)") << endL
        << endL
        << StringView(u8"Options:") << endL
        << StringView(u8"  --help         print this help") << endL
        << StringView(u8"  --threads=N    run tests in parallel using N threads") << endL
        << StringView(u8"  --OPT          equivalent to --OPT=1") << endL
        << StringView(u8"  --OPT=VALUE    set option OPT to VALUE") << endL
        << flsH;

    exit(0);
}

bool GetOpt::matchesFilter(StringView testName) const noexcept {
    if (matchesExclude(testName)) {
        return false;
    }

    if (includes.empty()) {
        return true;
    }

    return matchesFilterStrong(testName);
}

bool GetOpt::matchesFilterStrong(StringView testName) const noexcept {
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

bool GetOpt::matchesExclude(StringView testName) const noexcept {
    for (auto prefix : range(excludes)) {
        if (testName.startsWith(prefix)) {
            return true;
        }
    }

    return false;
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
