#include "ut.h"

#include <std/map/map.h>
#include <std/ios/sys.h>
#include <std/sym/i_map.h>
#include <std/ios/output.h>
#include <std/mem/obj_pool.h>

#include <stdlib.h>
#include <unistd.h>

#undef noexcept

#include <map>
#include <unordered_map>

using namespace stl;

STD_TEST_SUITE(UT) {
    STD_TEST(_Test1) {
        STD_INSIST(false);
    }

    STD_TEST(_Test2) {
        struct H: public Output {
            ZeroCopyOutput& out;

            H(ZeroCopyOutput& o) noexcept
                : out(o)
            {
            }

            size_t writeImpl(const void*, size_t len) override {
                out << len << endL;

                return len;
            }
        };

        H h(_ctx.output());
        OutBuf ob(h, 100);

        for (size_t i = 0; i < 1000; ++i) {
            ob << i;
        }
    }

}
