#pragma once

#include "future.h"
#include "future_iface.h"

#include <std/lib/producer.h>

namespace stl {
    struct CoroExecutor;
    struct ThreadPool;

    FutureIfaceRef asyncImpl(CoroExecutor* exec, ProducerIface* prod);
    FutureIfaceRef asyncImpl(ThreadPool* pool, ProducerIface* prod);

    template <typename F>
    auto async(CoroExecutor* exec, F fn) {
        return Future<decltype(fn())>{asyncImpl(exec, makeProducer(fn))};
    }

    template <typename F>
    auto async(ThreadPool* pool, F fn) {
        return Future<decltype(fn())>{asyncImpl(pool, makeProducer(fn))};
    }
}
