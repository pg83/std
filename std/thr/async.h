#pragma once

#include "future.h"
#include "future_iface.h"

#include <std/lib/producer.h>

namespace stl {
    struct CoroExecutor;

    FutureIfaceRef asyncImpl(CoroExecutor* exec, ProducerIface* prod);

    template <typename F>
    auto async(CoroExecutor* exec, F fn) {
        return Future<decltype(fn())>{asyncImpl(exec, makeProducer(fn))};
    }
}
