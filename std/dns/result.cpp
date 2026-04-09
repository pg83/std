#include "result.h"
#include "record.h"

#include <std/ios/outable.h>
#include <std/ios/out_zc.h>
#include <std/str/view.h>

using namespace stl;

template <>
void stl::output<ZeroCopyOutput, DnsResult*>(ZeroCopyOutput& out, DnsResult* res) {
    if (!res) {
        out << StringView(u8"(null)");

        return;
    }

    if (!res->ok()) {
        out << StringView(u8"error: ") << res->errorDescr();

        return;
    }

    out << res->record;
}

template <>
void stl::output<ZeroCopyOutput, const DnsResult*>(ZeroCopyOutput& out, const DnsResult* res) {
    output<ZeroCopyOutput, DnsResult*>(out, const_cast<DnsResult*>(res));
}
