#include "split_mix_64.h"

using namespace Std;

u64 Std::splitMix64(u64 x) noexcept {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9U;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebU;
    x ^= x >> 31;

    return x;
}

u64 Std::nextSplitMix64(u64* x) noexcept {
    return splitMix64(*x += 0x9e3779b97f4a7c15);
}
