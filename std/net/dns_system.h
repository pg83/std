#pragma once

namespace stl {
    class ObjPool;

    struct CoroExecutor;
    struct ThreadPool;
    struct DnsResolver;

    DnsResolver* createSystemDnsResolver(ObjPool* pool, CoroExecutor* exec, ThreadPool* tp);
}
