#include "out_buf.h"

#include "manip.h"
#include "output.h"
#include "unbound.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/alg/xchg.h>
#include <std/alg/defer.h>
#include <std/alg/minmax.h>
#include <std/alg/advance.h>

#include <sys/uio.h>

using namespace Std;

OutBuf::~OutBuf() {
    if (out_) {
        finish();
    }
}

OutBuf::OutBuf(Output& out) noexcept
    : out_(&out)
{
    if (!out.hint(&chunk)) {
        chunk = 1 << 16;
    }
}

OutBuf::OutBuf(Output& out, size_t chunkSize) noexcept
    : out_(&out)
    , chunk(chunkSize)
{
}

OutBuf::OutBuf() noexcept
    : out_(nullptr)
    , chunk(0)
{
}

void* OutBuf::imbueImpl(size_t* len) {
    return buf_.imbueMe(len);
}

void OutBuf::commitImpl(size_t len) noexcept {
    buf_.seekRelative(len);

    if (buf_.used() >= chunk) {
        Buffer buf;
        buf.xchg(buf_);
        write(buf.data(), buf.length());
    }
}

size_t OutBuf::writeDirect(const void* ptr, size_t len) {
    return out_->writeP(ptr, len - len % chunk);
}

size_t OutBuf::writeMultipart(const void* ptr, size_t len) {
    Buffer buf;

    buf.xchg(buf_);

    STD_DEFER {
        buf.reset();
        buf.xchg(buf_);
    };

    iovec parts[] = {
        {
            .iov_base = (void*)buf.data(),
            .iov_len = buf.used(),
        },
        {
            .iov_base = (void*)ptr,
            .iov_len = len - (buf.used() + len) % chunk,
        },
    };

    return out_->writeV(parts, 2);
}

size_t OutBuf::writeImpl(const void* ptr, size_t len) {
    if (buf_.used() + len < chunk) {
        return (buf_.append(ptr, len), len);
    } else if (const auto used = buf_.used(); used) {
        return writeMultipart(ptr, len) - used;
    } else {
        return writeDirect(ptr, len);
    }
}

size_t OutBuf::hintImpl() const noexcept {
    return chunk;
}

void OutBuf::flushImpl() {
    Buffer buf;

    buf.xchg(buf_);

    STD_DEFER {
        buf.reset();
        buf.xchg(buf_);
    };

    out_->write(buf.data(), buf.used());
}

void OutBuf::finishImpl() {
    STD_DEFER {
        out_ = nullptr;
    };

    flush();
}

void OutBuf::xchg(OutBuf& buf) noexcept {
    ::Std::xchg(buf_, buf.buf_);
    ::Std::xchg(out_, buf.out_);
    ::Std::xchg(chunk, buf.chunk);
}
