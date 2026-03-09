#include "mutex_iface.h"

using namespace stl;

MutexIface::~MutexIface() noexcept {
}

void* MutexIface::nativeHandle() noexcept {
    return nullptr;
}
