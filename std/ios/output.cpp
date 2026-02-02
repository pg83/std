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
    if (len < 1024) {
        if (!len) {
            return;
        }

        // do not call hint for really small data
        return writeImpl(data, len);
    }

    const u8* b = (u8*)data;
    const u8* e = b + len;

    while (true) {
        const auto part = hint();

        if (const auto left = e - b; left > part) {
            writeImpl(exchange(b, b + part), part);
        } else {
            return writeImpl(b, left);
        }
    }
}
