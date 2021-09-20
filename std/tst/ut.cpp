#include "ut.h"

#include <std/ios/sys.h>
#include <std/mem/pool.h>
#include <std/alg/qsort.h>
#include <std/alg/range.h>
#include <std/dbg/color.h>
#include <std/lib/vector.h>
#include <std/lib/singleton.h>

using namespace Std;

namespace {
    struct Test {
        StringView fullName;
        TestFunc* func;

        auto key() const noexcept {
            return func->suite().length() + func->name().length();
        }
    };

    struct GetOpt {
        bool listTests = false;
        Vector<StringView> includes;
    };

    struct Tests: public Vector<Test> {
        inline void run(int argc, char** argv) {
            Vector<const Test*> tests;

            for (auto& t : range(*this)) {
                tests.pushBack(&t);
            }

            quickSort(mutRange(tests), [](auto l, auto r) noexcept {
                return l->key() < r->key();
            });

            for (auto test : range(tests)) {
                sysE << Color::bright(AnsiColor::Yellow)
                     << StringView(u8"- ") << *test
                     << Color::reset() << finI;

                test->func->execute();

                sysE << Color::bright(AnsiColor::Green)
                     << StringView(u8"\r+ ") << *test
                     << Color::reset() << endL << finI;
            }
        }

        inline void reg(TestFunc* func) {
            pushBack(Test{.func = func});
        }

        static inline auto& instance() noexcept {
            return singleton<Tests>();
        }
    };
}

template <>
void Std::output<ZeroCopyOutput, Test>(ZeroCopyOutput& buf, const Test& test) {
    buf << test.func->suite() << StringView(u8"::") << test.func->name();
}

void Std::runTests(int argc, char** argv) {
    Tests::instance().run(argc, argv);
}

void Std::registerTest(TestFunc* test) {
    Tests::instance().reg(test);
}
