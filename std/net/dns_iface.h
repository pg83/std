#pragma once

#include <std/str/view.h>
#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct DnsConfig;
    struct DnsResult;
    struct ThreadPool;
    struct CoroExecutor;

    struct DnsResolver {
        virtual DnsResult* resolve(ObjPool* pool, const StringView& name) = 0;

        static DnsResolver* create(ObjPool* pool, CoroExecutor* exec);
        static DnsResolver* create(ObjPool* pool, CoroExecutor* exec, ThreadPool* tp);
        static DnsResolver* create(ObjPool* pool, CoroExecutor* exec, ThreadPool* tp, const DnsConfig& cfg);
    };
}
