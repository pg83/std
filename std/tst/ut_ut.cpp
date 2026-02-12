#include "ut.h"

#include <std/ios/sys.h>
#include <std/ios/output.h>

using namespace Std;

STD_TEST_SUITE(UT) {
    STD_TEST(_Test1) {
        STD_INSIST(false);
    }

    STD_TEST(_Test2) {
        struct H: public Output {
            size_t writeImpl(const void* buf, size_t len) override {
                sysE << len << endL << flsH;

                return len;
            }
        };

        H h;
        OutBuf ob(h, 100);

        for (size_t i = 0; i < 1000; ++i) {
            ob << i;
        }
    }
}
