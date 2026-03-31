#pragma once

#include <std/str/view.h>
#include <std/sys/types.h>

struct sockaddr;

namespace stl {
    class ObjPool;

    struct CoroExecutor;

    struct DnsResult {
        int error;
        sockaddr* addr;
        u32 addrLen;

        bool ok() const noexcept {
            return error == 0;
        }
    };

    struct DnsResolver {
        virtual DnsResult resolve(ObjPool* pool, const StringView& name) = 0;

        static DnsResolver* create(ObjPool* pool, CoroExecutor* exec);
    };
}
