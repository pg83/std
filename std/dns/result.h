#pragma once

#include <std/str/view.h>
#include <std/sys/types.h>

namespace stl {
    struct DnsRecord;

    struct DnsResult {
        int error;
        DnsRecord* record;

        bool ok() const noexcept {
            return error == 0;
        }

        virtual StringView errorDescr() const noexcept = 0;
    };
}
