#include "ut.h"

#include <std/ios/sys.h>
#include <std/ios/output.h>
#include <std/mem/obj_pool.h>

using namespace Std;

STD_TEST_SUITE(UT) {
    STD_TEST(_Test1) {
        STD_INSIST(false);
    }

    STD_TEST(_Test2) {
        struct H: public Output {
            ZeroCopyOutput& out;

            inline H(ZeroCopyOutput& o) noexcept
                : out(o)
            {
            }

            size_t writeImpl(const void* buf, size_t len) override {
                out << len << endL;

                return len;
            }
        };

        H h(ctx.output());
        OutBuf ob(h, 100);

        for (size_t i = 0; i < 1000; ++i) {
            ob << i;
        }
    }

    STD_TEST(_ObjPoolPerf) {
        volatile size_t res = 0;

        for (size_t i = 0; i < 100000000; ++i) {
            auto p = ObjPool::fromMemory();
            res += (size_t)p.ptr();
        }
    }
}
