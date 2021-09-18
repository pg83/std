#include "ut.h"

#include <std/ios/sys.h>
#include <std/alg/range.h>
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
                sysE << StringView(u8"- ") << test << finI;
                test.func->execute();
                sysE << StringView(u8"\r+ ") << test << endL << finI;
            }
        }

        static inline auto& instance() noexcept {
            return singleton<Tests>();
        }
    };
}

template <>
void Std::output<Test>(OutBuf& buf, const Test& test) {
    buf << test.suite << StringView(u8"::") << test.name;
}

void Std::runTests() {
    Tests::instance().run();
}

void Std::registerTest(const StringView& suite, const StringView& name, TestFunc* test) {
    Tests::instance().pushBack(Test{suite, name, test});
}
