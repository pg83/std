#include "ut.h"

#include <std/ios/sys.h>
#include <std/alg/range.h>
#include <std/dbg/color.h>
#include <std/lib/vector.h>
#include <std/lib/singleton.h>

using namespace Std;

namespace {
    struct Test {
        StringView suite;
        StringView name;
        TestFunc* func;
    };

    struct Tests: public Vector<Test>  {
        inline void run() {
            for (auto test : range(*this)) {
                sysE << Color::bright(AnsiColor::Yellow)
                     << StringView(u8"- ") << test
                     << Color::reset() << finI;

                test.func->execute();

                sysE << Color::bright(AnsiColor::Green)
                     << StringView(u8"\r+ ") << test
                     << Color::reset() << endL << finI;
            }
        }

        static inline auto& instance() noexcept {
            return singleton<Tests>();
        }
    };
}

template <>
void Std::output<ZeroCopyOutput, Test>(ZeroCopyOutput& buf, const Test& test) {
    buf << test.suite << StringView(u8"::") << test.name;
}

void Std::runTests() {
    Tests::instance().run();
}

void Std::registerTest(const StringView& suite, const StringView& name, TestFunc* test) {
    Tests::instance().pushBack(Test{suite, name, test});
}
