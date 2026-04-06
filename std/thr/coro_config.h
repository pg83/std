#pragma once

#include <std/sys/types.h>

namespace stl {
    struct CoroConfig {
        size_t threads;
        size_t reactors;
        size_t offloadThreads;
        size_t dnsResolvers;
        size_t maxDnsQueries;
        int dnsFamily;
        int dnsTimeout;
        int dnsTries;
        int dnsUdpMaxQueries;

        CoroConfig(size_t threads) noexcept;

        CoroConfig& setReactors(size_t v) noexcept;
        CoroConfig& setOffloadThreads(size_t v) noexcept;
        CoroConfig& setDnsResolvers(size_t v) noexcept;
        CoroConfig& setMaxDnsQueries(size_t v) noexcept;
        CoroConfig& setDnsFamily(int v) noexcept;
        CoroConfig& setDnsTimeout(int v) noexcept;
        CoroConfig& setDnsTries(int v) noexcept;
        CoroConfig& setDnsUdpMaxQueries(int v) noexcept;
    };
}
