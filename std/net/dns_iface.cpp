#include "dns_iface.h"
#include "dns_config.h"
#include "dns_ares.h"
#include "dns_system.h"

#include <std/thr/pool.h>
#include <std/mem/obj_pool.h>

#include <stdlib.h>

using namespace stl;

DnsResolver* DnsResolver::create(ObjPool* pool, CoroExecutor* exec) {
    return create(pool, exec, nullptr);
}

DnsResolver* DnsResolver::create(ObjPool* pool, CoroExecutor* exec, ThreadPool* tp) {
    return create(pool, exec, tp, DnsConfig());
}

DnsResolver* DnsResolver::create(ObjPool* pool, CoroExecutor* exec, ThreadPool* tp, const DnsConfig& cfg) {
    if (!getenv("USE_SYSTEM_RESOLVER")) {
        if (auto res = createAresResolver(pool, exec, cfg); res) {
            return res;
        }
    }

    if (!tp) {
        tp = ThreadPool::simple(pool, 4);
    }

    return createSystemDnsResolver(pool, exec, tp, cfg);
}
