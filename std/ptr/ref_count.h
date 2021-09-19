#pragma once

#include <std/sys/atomic.h>

namespace Std {
    class AtomicRefCount {
        int counter_;

    public:
        inline AtomicRefCount() noexcept {
            stdAtomicStore(&counter_, 0, MemoryOrder::Relaxed);
        }

        inline int ref() noexcept {
            return stdAtomicAddAndFetch(&counter_, 1, MemoryOrder::Acquire);
        }

        inline int refCount() const noexcept {
            return stdAtomicFetch(&counter_, MemoryOrder::Relaxed);
        }

        inline int unref() noexcept {
            return stdAtomicSubAndFetch(&counter_, 1, MemoryOrder::Release);
        }
    };
}
