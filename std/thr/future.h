#pragma once

#include "future_iface.h"

namespace stl {
    template <typename T>
    struct Future {
        FutureIfaceRef impl;

        T& wait() noexcept {
            return *(T*)impl->wait();
        }

        T* posted() noexcept {
            return (T*)impl->posted();
        }

        T* release() noexcept {
            return (T*)impl->release();
        }

        T* consume() noexcept {
            return (wait(), release());
        }
    };
}
