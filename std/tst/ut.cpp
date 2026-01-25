#include "ut.h"

#include <std/ios/sys.h>
#include <std/ios/mem.h>
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

        inline Test(TestFunc* _func, StringView _fullName) noexcept
            : func(_func)
            , fullName(_fullName)
        {
        }

        inline void execute() {
            sysE << Color::bright(AnsiColor::Yellow)
                 << StringView(u8"- ") << fullName
                 << Color::reset() << finI;

            func->execute();

            sysE << Color::bright(AnsiColor::Green)
                 << StringView(u8"\r+ ") << fullName
                 << Color::reset() << endL << finI;
        }
    };

    struct GetOpt {
        bool listTests = false;
        Vector<StringView> includes;
    };

    struct Tests: public Vector<Test*> {
        Pool::Ref pool = Pool::fromMemory();

        inline void run(int argc, char** argv) {
            quickSort(mutRange(*this), [](auto l, auto r) noexcept {
                return l->fullName < r->fullName;
            });

            for (auto test : range(*this)) {
                test->execute();
            }
        }

        inline void reg(TestFunc* func) {
            DynString str;
            pushBack(pool->make<Test>(func, pool->intern((StringOutput(str) << *func).str())));
            str.clear();
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
