#pragma once

#include <std/str/view.h>
#include <std/sys/types.h>
#include <std/dbg/insist.h>

namespace Std {
    struct TestFunc {
        virtual void execute() const = 0;
        virtual StringView suite() const = 0;
        virtual StringView name() const = 0;
    };

    void registerTest(TestFunc* test);
    void runTests(int argc, char** argv);
}

#define STD_TEST_SUITE(_name) \
    static const auto SUITE_NAME = ::Std::StringView(u8 ## #_name); namespace Suite_ ## _name

#define STD_TEST(_name) \
    static struct Test_ ## _name: public ::Std::TestFunc { \
        inline Test_ ## _name() {                          \
            ::Std::registerTest(this);                     \
        }                                                  \
        ::Std::StringView suite() const override {         \
            return SUITE_NAME;                             \
        }                                                  \
        ::Std::StringView name() const override {          \
            return u8 ## #_name;                           \
        }                                                  \
        void execute() const override;                     \
    } REG_ ## _name;                                       \
    void Test_ ## _name::execute() const
