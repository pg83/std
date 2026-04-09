#include "record.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/ios/out_zc.h>
#include <std/ios/outable.h>

#include <arpa/inet.h>
#include <netinet/in.h>

using namespace stl;

template <>
void stl::output<ZeroCopyOutput, DnsRecord>(ZeroCopyOutput& out, DnsRecord rec) {
    size_t avail = 64;
    auto buf = (char*)out.imbue(&avail);

    if (rec.family == AF_INET) {
        inet_ntop(AF_INET, &((const sockaddr_in*)rec.addr)->sin_addr, buf, avail);
    } else {
        inet_ntop(AF_INET6, &((const sockaddr_in6*)rec.addr)->sin6_addr, buf, avail);
    }

    out.commit(strLen((const u8*)buf));
}

template <>
void stl::output<ZeroCopyOutput, DnsRecord*>(ZeroCopyOutput& out, DnsRecord* rec) {
    if (!rec) {
        out << StringView(u8"(null)");

        return;
    }

    out << *rec;

    if (rec->next) {
        out << StringView(u8", ") << rec->next;
    }
}

template <>
void stl::output<ZeroCopyOutput, const DnsRecord*>(ZeroCopyOutput& out, const DnsRecord* rec) {
    output<ZeroCopyOutput, DnsRecord*>(out, const_cast<DnsRecord*>(rec));
}
