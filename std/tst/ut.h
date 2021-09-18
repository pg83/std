#pragma once

#include <std/str/view.h>
#include <std/sys/types.h>

namespace Std {
    struct TestFunc {
        virtual void execute() = 0;
    };

    void registerTest(const StringView& suite, const StringView& name, TestFunc* test);
    void runTests();
}

#define STD_TEST_SUITE(name) \
    static const auto SUITE_NAME = ::Std::StringView(u8 ## #name); namespace meth

#define STD_TEST(name) \
    static struct name: public ::Std::TestFunc { \
        inline name() { \
            ::Std::registerTest(SUITE_NAME, u8 ## #name, this); \
        }\
        void execute() override;\
    } REG;\
    void name::execute() \
