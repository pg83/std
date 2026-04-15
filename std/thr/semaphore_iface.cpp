#include "semaphore_iface.h"

using namespace stl;

SemaphoreIface::~SemaphoreIface() noexcept {
}

void* SemaphoreIface::nativeHandle() noexcept {
    return nullptr;
}

bool SemaphoreIface::owned() const noexcept {
    return false;
}
