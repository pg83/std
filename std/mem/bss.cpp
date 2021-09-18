#include "bss.h"

namespace {
    alignas(128) static const char BSS[1024] = {};
}

const void* Std::bss() noexcept {
    return BSS;
}
