#include "split_mix_64.h"

using namespace Std;

u64 Std::nextSplitMix64(u64* x) noexcept {
    u64 z = (*x += 0x9e3779b97f4a7c15);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
}
