#include "stable.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(StringHash) {
    STD_TEST(test1) {
        SymbolTable s;

        s.set(u8"qw1", (void*)1);
        s.set(u8"qw2", (void*)2);

        STD_INSIST(s.find(u8"qw1") == (void*)1);
        STD_INSIST(s.find(u8"qw2") == (void*)2);
        STD_INSIST(s.find(u8"qw3") == nullptr);
    }
}
