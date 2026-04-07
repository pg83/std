#pragma once

#include <std/sys/types.h>

struct sockaddr;

namespace stl {
    struct DnsRecord {
        DnsRecord* next;
        int family;
        sockaddr* addr;
    };
}
