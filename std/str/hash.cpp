#include "hash.h"

#include <xxhash.h>

using namespace Std;

u32 Std::shash32(const void* data, size_t len) {
    return XXH32(data, len, 0);
}

u64 Std::shash64(const void* data, size_t len) {
    return XXH64(data, len, 0);
}
