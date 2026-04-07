#pragma once

#include <std/str/view.h>
#include <std/sys/types.h>

namespace stl {
    struct DnsConfig {
        size_t family;
        size_t timeout;
        size_t tries;
        size_t udpMaxQueries;
        bool tcp;
        StringView server;

        DnsConfig() noexcept;
    };
}
