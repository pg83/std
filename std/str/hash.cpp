#include "hash.h"

#include <rapidhash.h>

using namespace Std;

u32 Std::shash32(const void* data, size_t len) noexcept {
    auto h = shash64(data, len);

    return (u32)h ^ (u32)(h >> 32);
}

u64 Std::shash64(const void* data, size_t len) noexcept {
    return rapidhash(data, len);
}
