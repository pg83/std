#include "arc.h"
#include "shared.h"

#include <std/tst/ut.h>
#include <std/ios/sys.h>

using namespace Std;

STD_TEST_SUITE(SharedPtr) {
    STD_TEST(testOps) {
        auto v = AtomicSharedPtr<u32>::make();

        *v = 0;

        STD_INSIST(*v == 0);
        STD_INSIST(v.refCount() == 1);

        {
            auto c = v;

            *c = 1;

            STD_INSIST(*c == 1);
            STD_INSIST(*v == 1);

            STD_INSIST(v.refCount() == 2);
            STD_INSIST(c.refCount() == 2);

            STD_INSIST(&*c == &*v);
        }

        STD_INSIST(v.refCount() == 1);
        STD_INSIST(*v == 1);
    }
}
