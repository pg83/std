#include "output.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/alg/range.h>
#include <std/alg/exchange.h>

#include <alloca.h>
#include <sys/uio.h>

using namespace Std;

Output::~Output() noexcept {
}

void Output::flushImpl() {
}

void Output::finishImpl() {
}

void Output::writeVImpl(iovec* parts, size_t count) {
    for (const auto& it : range(parts, parts + count)) {
        write(it.iov_base, it.iov_len);
    }
}

void Output::writeV(const StringView* parts, size_t count) {
    auto io = (iovec*)alloca(count * sizeof(iovec));

    memZero(io, io + count);

    for (size_t i = 0; i < count; ++i) {
        io[i].iov_len = parts[i].length();
        io[i].iov_base = (void*)parts[i].data();
    }

    writeV(io, count);
}

void Output::write(const void* data, size_t len) {
    auto b = (u8*)data;
    auto e = b + len;

    while (b < e) {
        if (const auto left = e - b; left < 1024) {
            b += writeImpl(b, left);
        } else if (const auto part = hint(); left > part) {
            b += writeImpl(b, part);
        } else {
            b += writeImpl(b, left);
        }
    }
}
