#include <std/rng/pcg.h>
#include <std/alg/xchg.h>
#include <std/alg/qsort.h>
#include <std/alg/range.h>
#include <std/lib/vector.h>
#include <std/mem/pool.h>
#include <std/ios/sys.h>
#include <std/str/view.h>

#include <algorithm>

using namespace Std;

static u32 swaps = 0;
static u32 compares = 0;
static u32 copies = 0;

#define noinline __attribute__((__noinline__))

struct Int {
    u64 v;

    noinline Int(u64 _v)
        : v(_v)
    {
    }

    noinline Int(const Int& _v)
        : v(_v.v)
    {
        ++copies;
    }

    noinline Int& operator=(const Int& _v) noexcept {
        v = _v.v;

        ++copies;

        return *this;
    }

    noinline void xchg(Int& v) noexcept {
        Std::xchg(this->v, v.v);
        ++swaps;
    }

    friend noinline bool operator<(const Int& l, const Int& r) noexcept {
        ++compares;
        return l.v < r.v;
    }

    friend noinline void swap(Int& l, Int& r) noexcept {
        l.xchg(r);
    }
};

int sortMain(int argc, char** argv) {
    PCG32 p(100);

    if (1) {
        Vector<Int> v;

        for (size_t i = 0; i < 1000; ++i) {
            v.pushBack(p.nextU32());
        }

        quickSort(mutRange(v));
        //std::sort(v.mutBegin(), v.mutEnd());

        sysE << swaps << StringView(u8" ")
             << compares << StringView(u8" ")
             << copies << endL << finI;
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
