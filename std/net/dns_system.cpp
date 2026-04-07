#include "dns_system.h"
#include "dns_config.h"
#include "dns_record.h"
#include "dns_result.h"
#include "dns_iface.h"

#include <std/sys/crt.h>
#include <std/thr/coro.h>
#include <std/thr/pool.h>
#include <std/alg/defer.h>
#include <std/mem/obj_pool.h>

#include <netdb.h>
#include <string.h>
#include <sys/socket.h>

using namespace stl;

namespace {
    struct DnsSysRecordImpl: public DnsRecord {
        DnsSysRecordImpl(ObjPool* pool, struct addrinfo* node);
    };

    struct DnsSysResultImpl: public DnsResult {
        DnsSysResultImpl(ObjPool* pool, int gaierr, struct addrinfo* ai);
        StringView errorDescr() const noexcept override;
    };

    struct DnsSysResolverImpl: public DnsResolver {
        CoroExecutor* exec_;
        ThreadPool* tp_;
        struct addrinfo hints_;

        DnsSysResolverImpl(CoroExecutor* exec, ThreadPool* tp, const DnsConfig& cfg);

        DnsResult* resolve(ObjPool* pool, const StringView& name) override;
    };
}

DnsSysRecordImpl::DnsSysRecordImpl(ObjPool* pool, struct addrinfo* node) {
    family = node->ai_family;
    addr = node->ai_addr;

    if (node->ai_next) {
        next = pool->make<DnsSysRecordImpl>(pool, node->ai_next);
    } else {
        next = nullptr;
    }
}

StringView DnsSysResultImpl::errorDescr() const noexcept {
    return gai_strerror(error);
}

DnsSysResultImpl::DnsSysResultImpl(ObjPool* pool, int gaierr, struct addrinfo* ai) {
    if (ai) {
        atExit(pool, [ai] {
            freeaddrinfo(ai);
        });
    }

    if (gaierr == EAI_NONAME) {
        error = 0;
        record = nullptr;
    } else if (gaierr != 0) {
        error = gaierr;
        record = nullptr;
    } else if (!ai) {
        error = 0;
        record = nullptr;
    } else {
        error = 0;
        record = pool->make<DnsSysRecordImpl>(pool, ai);
    }
}

DnsSysResolverImpl::DnsSysResolverImpl(CoroExecutor* exec, ThreadPool* tp, const DnsConfig& cfg)
    : exec_(exec)
    , tp_(tp)
{
    memset(&hints_, 0, sizeof(hints_));
    hints_.ai_family = cfg.family;
    hints_.ai_socktype = SOCK_STREAM;
}

DnsResult* DnsSysResolverImpl::resolve(ObjPool* pool, const StringView& name) {
    const char* host = (const char*)pool->intern(name).data();
    struct addrinfo* ai = nullptr;
    int gaierr = 0;

    exec_->offload(tp_, [&] {
        gaierr = getaddrinfo(host, nullptr, &hints_, &ai);
    });

    return pool->make<DnsSysResultImpl>(pool, gaierr, ai);
}

stl::DnsResolver* stl::createSystemDnsResolver(ObjPool* pool, CoroExecutor* exec, ThreadPool* tp, const DnsConfig& cfg) {
    return pool->make<DnsSysResolverImpl>(exec, tp, cfg);
}
