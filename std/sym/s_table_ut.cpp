#include "s_table.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(SymbolTable) {
    STD_TEST(test1) {
        SymbolTable s;

        s.set("qw1", (void*)1);
        s.set("qw2", (void*)2);

        STD_INSIST(s.find("qw1") == (void*)1);
        STD_INSIST(s.find("qw2") == (void*)2);
        STD_INSIST(s.find("qw3") == nullptr);
    }
}
