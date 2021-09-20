#include <std/tst/ut.h>
#include <std/alg/qsort.h>
#include <std/rng/pcg.h>

#include <algorithm>
#include <vector>

using namespace Std;

template <typename T>
auto sum(const T& t) {
    u64 r = 0;

    for (auto v : t) {
        r += v;
    }

    return r;
}

int main(int argc, char** argv) {
    runTests(argc, argv);

    return 0;

    PCG32 p(100);

    for (size_t n = 0; n < 1000; ++n) {
        std::vector<u64> v;

        for (size_t i = 0; i < 10000; ++i) {
            v.push_back(p.nextU32());
            //v.push_back(i);
        }

        //auto sb = sum(v);

        //std::sort(v.begin(), v.end());
        quickSort(v.data(), v.data() + v.size());

        //STD_INSIST(sb == sum(v));
        //STD_INSIST(v[0] < v.back());
    }
}
