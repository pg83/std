#include "arc.h"

#include <std/sys/atomic.h>

using namespace Std;

ARC::ARC() noexcept {
    stdAtomicStore(&counter_, 0, MemoryOrder::Relaxed);
}

int ARC::ref() noexcept {
    return stdAtomicAddAndFetch(&counter_, 1, MemoryOrder::Acquire);
}

int ARC::refCount() const noexcept {
    return stdAtomicFetch(&counter_, MemoryOrder::Relaxed);
}

int ARC::unref() noexcept {
    return stdAtomicSubAndFetch(&counter_, 1, MemoryOrder::Release);
}
