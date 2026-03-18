#pragma once

#include "future_iface.h"

#include <std/sys/types.h>
#include <std/lib/producer.h>

namespace stl {
    struct CoroExecutor;

    FutureIfaceRef asyncImpl(CoroExecutor* exec, ProducerIface* prod);

    template <typename T>
    struct SharedFuture {
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

    template <typename F>
    auto async(CoroExecutor* exec, F fn) {
        return SharedFuture<decltype(fn())>{asyncImpl(exec, makeProducer(fn))};
    }
}
