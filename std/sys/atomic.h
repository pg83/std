#pragma once

#define stdAtomicAddAndFetch __atomic_add_fetch
#define stdAtomicSubAndFetch __atomic_sub_fetch
#define stdAtomicFetch __atomic_load_n
#define stdAtomicStore __atomic_store_n

namespace Std {
    namespace MemoryOrder {
        constexpr int Acquire = __ATOMIC_ACQUIRE;
        constexpr int Release = __ATOMIC_RELEASE;
        constexpr int Consume = __ATOMIC_CONSUME;
        constexpr int Relaxed = __ATOMIC_RELAXED;
    };
}
