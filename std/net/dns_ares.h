#pragma once

namespace stl {
    class ObjPool;

    struct DnsConfig;
    struct DnsResolver;
    struct CoroExecutor;

    DnsResolver* createAresResolver(ObjPool* pool, CoroExecutor* exec, DnsConfig cfg);
}
