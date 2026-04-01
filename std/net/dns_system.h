#pragma once

namespace stl {
    class ObjPool;

    struct ThreadPool;
    struct DnsResolver;
    struct CoroExecutor;

    DnsResolver* createSystemDnsResolver(ObjPool* pool, CoroExecutor* exec, ThreadPool* tp);
}
