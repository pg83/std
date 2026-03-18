#pragma once

#include "future.h"
#include "future_iface.h"

#include <std/lib/producer.h>

namespace stl {
    struct ThreadPool;
    struct CoroExecutor;

    FutureIfaceRef asyncImpl(ProducerIface* prod);
    FutureIfaceRef asyncImpl(ProducerIface* prod, ThreadPool* pool);
    FutureIfaceRef asyncImpl(ProducerIface* prod, CoroExecutor* exec);

    template <typename F>
    auto async(F fn) {
        return Future<decltype(fn())>{asyncImpl(makeProducer(fn))};
    }

    template <typename F>
    auto async(CoroExecutor* exec, F fn) {
        return Future<decltype(fn())>{asyncImpl(makeProducer(fn), exec)};
    }

    template <typename F>
    auto async(ThreadPool* pool, F fn) {
        return Future<decltype(fn())>{asyncImpl(makeProducer(fn), pool)};
    }
}
