#pragma once

#include <std/str/view.h>
#include <std/sys/types.h>
#include <std/dbg/insist.h>

namespace Std {
    struct TestFunc {
        virtual void execute() const = 0;
    };

    void registerTest(const StringView& suite, const StringView& name, TestFunc* test);
    void runTests();
}

#define STD_TEST_SUITE(name) \
    static const auto SUITE_NAME = ::Std::StringView(u8 ## #name); namespace Suite_ ## name

#define STD_TEST(name)                                          \
    static struct Test_ ## name: public ::Std::TestFunc {       \
        inline Test_ ## name() {                                \
            ::Std::registerTest(SUITE_NAME, u8 ## #name, this); \
        }                                                       \
        void execute() const override;                          \
    } REG_ ## name;                                             \
    void Test_ ## name::execute() const
