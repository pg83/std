#include "arc.h"

#include <std/sys/atomic.h>

using namespace stl;

ARC::ARC() {
    stdAtomicStore(&counter_, 0, MemoryOrder::Relaxed);
}

i32 ARC::ref() {
    return stdAtomicAddAndFetch(&counter_, 1, MemoryOrder::Acquire);
}

i32 ARC::refCount() const {
    return stdAtomicFetch(&counter_, MemoryOrder::Acquire);
}

i32 ARC::unref() {
    return stdAtomicSubAndFetch(&counter_, 1, MemoryOrder::Release);
}
