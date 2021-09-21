#include "ops.h"

#include <std/sys/crt.h>

using namespace Std;

int Std::spaceship(const u8* l, size_t ll, const u8* r, size_t rl) noexcept {
    const auto rr = memCmp(l, r, ll < rl ? ll : rl);

    return rr ? rr : (ll < rl ? -1 : (ll == rl ? 0 : 1));
}
