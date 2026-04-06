#pragma once

#include <std/str/view.h>
#include <std/sys/types.h>

struct sockaddr;

namespace stl {
    class ObjPool;

    struct ThreadPool;
    struct CoroExecutor;

    struct DnsConfig {
        size_t family;
        size_t timeout;
        size_t tries;
        size_t udpMaxQueries;

        DnsConfig() noexcept;
    };

    struct DnsRecord {
        DnsRecord* next;
        int family;
        sockaddr* addr;
    };

    struct DnsResult {
        int error;
        DnsRecord* record;

        bool ok() const noexcept {
            return error == 0;
        }

        virtual StringView errorDescr() const noexcept = 0;
    };

    struct DnsResolver {
        virtual DnsResult* resolve(ObjPool* pool, const StringView& name) = 0;

        static DnsResolver* create(ObjPool* pool, CoroExecutor* exec);
        static DnsResolver* create(ObjPool* pool, CoroExecutor* exec, ThreadPool* tp);
        static DnsResolver* create(ObjPool* pool, CoroExecutor* exec, ThreadPool* tp, DnsConfig cfg);
    };
}
