#include "out_null.h"
#include "in_mem.h"

#include <std/tst/ut.h>

using namespace stl;

namespace {
    static void copy(Input& in, Output& out) {
        in.sendTo(out);
    }
}

STD_TEST_SUITE(NullOutput) {
    STD_TEST(EmptyWrite) {
        NullOutput output;
        size_t bytesWritten = output.write("", 0);
        STD_INSIST(bytesWritten == 0);
    }

    STD_TEST(SmallWrite) {
        NullOutput output;
        size_t bytesWritten = output.write("hello", 5);
        STD_INSIST(bytesWritten == 5);
    }

    STD_TEST(LargeWrite) {
        NullOutput output;
        u8 data[10000];
        size_t bytesWritten = output.write(data, sizeof(data));
        STD_INSIST(bytesWritten == sizeof(data));
    }

    STD_TEST(MultipleWrites) {
        NullOutput output;
        size_t total = 0;

        for (size_t i = 0; i < 100; ++i) {
            total += output.write("data", 4);
        }

        STD_INSIST(total == 400);
    }

    STD_TEST(FlushAndFinish) {
        NullOutput output;
        output.write("test", 4);
        output.flush();
        output.finish();
    }

    STD_TEST(CopyFromInput) {
        const char* data = "hello world";
        MemoryInput input(data, 11);
        NullOutput output;
        copy(input, output);
    }
}
