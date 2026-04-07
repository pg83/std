#include "config.h"

#include <sys/socket.h>

using namespace stl;

DnsConfig::DnsConfig() noexcept
    : family(AF_UNSPEC)
    , timeout(100)
    , tries(3)
    , udpMaxQueries(0)
    , tcp(false)
    , server()
{
}
