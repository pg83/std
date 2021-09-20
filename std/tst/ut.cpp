#include "ut.h"

#include <std/ios/sys.h>
#include <std/ios/string.h>

#include <std/mem/pool.h>

#include <std/str/view.h>
#include <std/str/dynamic.h>

#include <std/alg/qsort.h>
#include <std/alg/range.h>

#include <std/dbg/color.h>

#include <std/lib/vector.h>
#include <std/lib/singleton.h>

using namespace Std;

namespace {
    struct Test {
        TestFunc* func;
        StringView fullName;

        auto key() const noexcept {
            return fullName.length();
        }
    };

    struct GetOpt {
        bool listTests = false;
        Vector<StringView> includes;
    };

    struct Tests: public Vector<Test> {
        DynString tmp;
        Pool::Ref pool = Pool::fromMemory();

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
                     << StringView(u8"- ") << test->fullName
                     << Color::reset() << finI;

                test->func->execute();

                sysE << Color::bright(AnsiColor::Green)
                     << StringView(u8"\r+ ") << test->fullName
                     << Color::reset() << endL << finI;
            }
        }

        inline void reg(TestFunc* func) {
            tmp.clear();

            StringOutput(tmp) << *func;

            pushBack(Test{.func = func, .fullName = pool->intern(tmp)});
        }

        static inline auto& instance() noexcept {
            return singleton<Tests>();
        }
    };
}

template <>
void Std::output<ZeroCopyOutput, TestFunc>(ZeroCopyOutput& buf, const TestFunc& test) {
    buf << test.suite() << StringView(u8"::") << test.name();
}

void Std::runTests(int argc, char** argv) {
    Tests::instance().run(argc, argv);
}

void Std::registerTest(TestFunc* test) {
    Tests::instance().reg(test);
}
