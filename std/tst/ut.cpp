#include "ut.h"

#include <std/alg/range.h>
#include <std/ios/output.h>
#include <std/lib/vector.h>
#include <std/lib/singleton.h>

using namespace Std;

namespace {
    struct Tests {
        struct Test {
            StringView suite;
            StringView name;
            TestFunc* func;
        };

        Vector<Test> tests;

        inline void reg(const StringView& suite, const StringView& name, TestFunc* func) {
            tests.pushBack(Test{suite, name, func});
        }

        inline void run() {
            for (auto test : range(tests)) {
                stdE << test.suite << StringView(u8"::") << test.name << endL;

                test.func->execute();
            }
        }

        static inline auto& instance() noexcept {
            return singleton<Tests>();
        }
    };
}

void Std::runTests() {
    Tests::instance().run();
}

void Std::registerTest(const StringView& suite, const StringView& name, TestFunc* test) {
    Tests::instance().reg(suite, name, test);
}
