#include "bss.h"

namespace {
    alignas(128) static const char BSS[4096] = {0};
}

const void* Std::bss() noexcept {
    return BSS;
}
