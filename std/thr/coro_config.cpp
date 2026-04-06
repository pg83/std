#include "coro_config.h"

#include <std/net/dns_iface.h>

using namespace stl;

CoroConfig::CoroConfig(size_t threads) noexcept
    : threads(threads)
    , reactors(threads)
    , offloadThreads(threads)
    , dnsResolvers(threads)
    , maxDnsQueries(100)
    , dnsFamily(DnsConfig().family)
    , dnsTimeout(100)
    , dnsTries(3)
    , dnsUdpMaxQueries(0)
{
}

CoroConfig& CoroConfig::setReactors(size_t v) noexcept {
    reactors = v;

    return *this;
}

CoroConfig& CoroConfig::setOffloadThreads(size_t v) noexcept {
    offloadThreads = v;

    return *this;
}

CoroConfig& CoroConfig::setDnsResolvers(size_t v) noexcept {
    dnsResolvers = v;

    return *this;
}

CoroConfig& CoroConfig::setMaxDnsQueries(size_t v) noexcept {
    maxDnsQueries = v;

    return *this;
}

CoroConfig& CoroConfig::setDnsFamily(int v) noexcept {
    dnsFamily = v;

    return *this;
}

CoroConfig& CoroConfig::setDnsTimeout(int v) noexcept {
    dnsTimeout = v;

    return *this;
}

CoroConfig& CoroConfig::setDnsTries(int v) noexcept {
    dnsTries = v;

    return *this;
}

CoroConfig& CoroConfig::setDnsUdpMaxQueries(int v) noexcept {
    dnsUdpMaxQueries = v;

    return *this;
}
