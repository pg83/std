#include "arc.h"
#include "intrusive.h"

#include <std/tst/ut.h>

using namespace Std;

STD_TEST_SUITE(IntrusivePtr) {
    struct Int: public ARC {
        int v = 1;
    };

    STD_TEST(testOps) {
        IntrusivePtr<Int> v(new Int());

        STD_INSIST(v->refCount() == 1);
        STD_INSIST(v->v == 1);

        {
            auto c = v;

            STD_INSIST(v->refCount() == 2);
            STD_INSIST(c->refCount() == 2);

            STD_INSIST(v->v == 1);
            STD_INSIST(c->v == 1);

            c->v = 2;

            STD_INSIST(v->v == 2);
            STD_INSIST(c->v == 2);
        }

        STD_INSIST(v->refCount() == 1);
        STD_INSIST(v->v == 2);
    }
}
