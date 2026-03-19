#include "semaphore_iface.h"

using namespace stl;

SemaphoreIface::~SemaphoreIface() noexcept {
}

void* SemaphoreIface::nativeHandle() noexcept {
    return nullptr;
}
