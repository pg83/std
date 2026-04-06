#pragma once

#include <std/str/view.h>
#include <std/sys/types.h>

namespace stl {
    struct CoroConfig {
        size_t threads;
        size_t reactors;
        size_t offloadThreads;
        size_t dnsResolvers;
        size_t maxDnsQueries;
        size_t dnsFamily;
        size_t dnsTimeout;
        size_t dnsTries;
        size_t dnsUdpMaxQueries;
        bool dnsTcp;
        StringView dnsServer;

        CoroConfig(size_t threads) noexcept;

        CoroConfig& setThreads(size_t v) noexcept;
        CoroConfig& setReactors(size_t v) noexcept;
        CoroConfig& setOffloadThreads(size_t v) noexcept;
        CoroConfig& setDnsResolvers(size_t v) noexcept;
        CoroConfig& setMaxDnsQueries(size_t v) noexcept;
        CoroConfig& setDnsFamily(size_t v) noexcept;
        CoroConfig& setDnsTimeout(size_t v) noexcept;
        CoroConfig& setDnsTries(size_t v) noexcept;
        CoroConfig& setDnsUdpMaxQueries(size_t v) noexcept;
        CoroConfig& setDnsTcp(bool v) noexcept;
        CoroConfig& setDnsServer(StringView v) noexcept;
    };
}
