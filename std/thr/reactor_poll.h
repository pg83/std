#pragma once

#include "pollable.h"

namespace stl {
    class ObjPool;

    struct PollerIface;
    struct CoroExecutor;

    struct ReactorIface: public Pollable {
        virtual PollerIface* createPoller(ObjPool* pool) = 0;

        static ReactorIface* create(CoroExecutor* exec, ObjPool* opool);
    };
}
