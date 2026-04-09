#include "record.h"

#include <std/ios/outable.h>
#include <std/ios/out_zc.h>
#include <std/str/view.h>

#include <arpa/inet.h>
#include <netinet/in.h>

using namespace stl;

template <>
void stl::output<ZeroCopyOutput, DnsRecord*>(ZeroCopyOutput& out, DnsRecord* rec) {
    if (!rec) {
        out << StringView(u8"(null)");

        return;
    }

    char buf[64];

    if (rec->family == AF_INET) {
        inet_ntop(AF_INET, &((const sockaddr_in*)rec->addr)->sin_addr, buf, sizeof(buf));
    } else {
        inet_ntop(AF_INET6, &((const sockaddr_in6*)rec->addr)->sin6_addr, buf, sizeof(buf));
    }

    out << StringView(buf);

    if (rec->next) {
        out << StringView(u8", ") << rec->next;
    }
}

template <>
void stl::output<ZeroCopyOutput, const DnsRecord*>(ZeroCopyOutput& out, const DnsRecord* rec) {
    output<ZeroCopyOutput, DnsRecord*>(out, const_cast<DnsRecord*>(rec));
}
