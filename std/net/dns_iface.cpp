#include "dns_iface.h"
#include "dns_ares.h"
#include "dns_system.h"

#include <std/thr/pool.h>
#include <std/mem/obj_pool.h>

using namespace stl;

DnsResolver* DnsResolver::create(ObjPool* pool, CoroExecutor* exec) {
    if (auto res = createAresResolver(pool, exec); res) {
        return res;
    }

    return createSystemDnsResolver(pool, exec, ThreadPool::simple(pool, 4));
}
