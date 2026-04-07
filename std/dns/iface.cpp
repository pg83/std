#include "iface.h"
#include "ares.h"
#include "system.h"

#include <std/thr/pool.h>
#include <std/mem/obj_pool.h>

#include <stdlib.h>

using namespace stl;

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
