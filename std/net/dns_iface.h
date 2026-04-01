#pragma once

#include <std/str/view.h>
#include <std/sys/types.h>

struct sockaddr;

namespace stl {
    class ObjPool;

    struct CoroExecutor;

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
    };
}
