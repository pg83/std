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

void Output::writeV(iovec* iov, size_t iovcnt) {
    while (iovcnt > 0) {
        auto written = writeVImpl(iov, iovcnt);

        while (written >= iov->iov_len) {
            written -= iov->iov_len;
            iov++;
            iovcnt--;
        }

        if (written > 0) {
            iov->iov_base = (char*)iov->iov_base + written;
            iov->iov_len -= written;
        }
    }
}

size_t Output::writeVImpl(iovec* parts, size_t count) {
    size_t res = 0;

    for (const auto& it : range(parts, parts + count)) {
        write(it.iov_base, it.iov_len);
        res += it.iov_len;
    }

    return res;
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
        } else if (size_t part; hint(&part) && left > part) {
            b += writeImpl(b, part);
        } else {
            b += writeImpl(b, left);
        }
    }
}

size_t Output::hintImpl() const noexcept {
    return 0;
}

bool Output::hint(size_t* res) const noexcept {
    if (const auto h = hintImpl(); h) {
        *res = h;

        return true;
    }

    return false;
}
