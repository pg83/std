#include <std/tst/ut.h>
#include <std/rng/pcg.h>
#include <std/alg/qsort.h>
#include <std/alg/range.h>
#include <std/lib/vector.h>

#include <algorithm>

using namespace Std;

int main(int argc, char** argv) {
    runTests(argc, argv);

    return 0;

    PCG32 p(100);

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
