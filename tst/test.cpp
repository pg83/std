#include <std/tst/ut.h>
#include <std/rng/pcg.h>
#include <std/alg/xchg.h>
#include <std/alg/qsort.h>
#include <std/alg/range.h>
#include <std/lib/vector.h>
#include <std/mem/pool.h>
#include <std/ios/sys.h>
#include <algorithm>

using namespace Std;

static u32 swaps = 0;
static u32 compares = 0;

struct Int {
    u64 v;

    inline Int(u64 _v)
        : v(_v)
    {
    }

    inline void xchg(Int& v) noexcept {
        Std::xchg(this->v, v.v);
        ++swaps;
    }

    friend inline bool operator<(const Int& l, const Int& r) noexcept {
        ++compares;
        return l.v < r.v;
    }
};

int main(int argc, char** argv) {
    runTests(argc, argv);

    PCG32 p(100);

    if (1) {
        Vector<Int> v;

        for (size_t i = 0; i < 10000; ++i) {
            v.pushBack(p.nextU32());
        }

        quickSort(mutRange(v));

        sysE << swaps << StringView(u8" ") << compares << endL << finI;
    }

    if (1) {
        for (size_t n = 0; n < 10000; ++n) {
            Vector<u64> v;

            for (size_t i = 0; i < 10000; ++i) {
                v.pushBack(p.nextU32());
                //v.pushBack(i);
            }

            //std::sort(v.mutBegin(), v.mutEnd());
            quickSort(mutRange(v));
        }
    }

    return 0;
}
