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
    , dnsTcp(false)
    , dnsServer()
{
}

CoroConfig& CoroConfig::setThreads(size_t v) noexcept {
    threads = v;

    return *this;
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

CoroConfig& CoroConfig::setDnsFamily(size_t v) noexcept {
    dnsFamily = v;

    return *this;
}

CoroConfig& CoroConfig::setDnsTimeout(size_t v) noexcept {
    dnsTimeout = v;

    return *this;
}

CoroConfig& CoroConfig::setDnsTries(size_t v) noexcept {
    dnsTries = v;

    return *this;
}

CoroConfig& CoroConfig::setDnsUdpMaxQueries(size_t v) noexcept {
    dnsUdpMaxQueries = v;

    return *this;
}

CoroConfig& CoroConfig::setDnsTcp(bool v) noexcept {
    dnsTcp = v;

    return *this;
}

CoroConfig& CoroConfig::setDnsServer(StringView v) noexcept {
    dnsServer = v;

    return *this;
}
