#pragma once

namespace stl {
    class ObjPool;

    struct DnsResolver;
    struct CoroExecutor;

    DnsResolver* createAresResolver(ObjPool* pool, CoroExecutor* exec);
}
