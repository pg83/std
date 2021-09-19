#include "arc.h"

#include <std/sys/atomic.h>

using namespace Std;

ARC::ARC() noexcept {
    stdAtomicStore(&counter_, 0, MemoryOrder::Relaxed);
}

i32 ARC::ref() noexcept {
    return stdAtomicAddAndFetch(&counter_, 1, MemoryOrder::Acquire);
}

i32 ARC::refCount() const noexcept {
    return stdAtomicFetch(&counter_, MemoryOrder::Relaxed);
}

i32 ARC::unref() noexcept {
    return stdAtomicSubAndFetch(&counter_, 1, MemoryOrder::Release);
}
