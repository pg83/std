#include "scratch.h"

namespace {
    alignas(128) static char SCRATCH[1024];
}

void* Std::scratchMem() noexcept {
    return SCRATCH;
}
