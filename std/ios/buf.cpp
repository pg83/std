#include "buf.h"
#include "manip.h"
#include "output.h"
#include "unbound.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/alg/minmax.h>

using namespace Std;
using namespace Std::Manip;

OutBuf::~OutBuf() {
    if (out_) {
        finish();
    }
}

OutBuf::OutBuf(Output& out, size_t chunkSize) noexcept
    : out_(&out)
    , chunk(min(chunkSize, out_->hint()))
{
    buf_.grow(256);
}

OutBuf::OutBuf() noexcept
    : out_(nullptr)
    , chunk(0)
{
}

void* OutBuf::imbueImpl(size_t len) {
    buf_.growDelta(len);

    return (void*)buf_.mutCurrent();
}

void OutBuf::bumpImpl(const void* ptr) noexcept {
    buf_.seekAbsolute(ptr);

    if (buf_.used() >= chunk) {
        flush();
    }
}

void OutBuf::writeImpl(const void* ptr, size_t len) {
    if (buf_.used() + len < chunk) {
        buf_.append(ptr, len);
    } else if (buf_.used()) {
        Buffer buf;

        buf.xchg(buf_);

        const StringView parts[] = {
            StringView((const u8*)buf.data(), buf.used()),
            StringView((const u8*)ptr, len),
        };

        out_->writeV(parts, 2);
    } else {
        out_->write(ptr, len);
    }
}

size_t OutBuf::hintImpl() const noexcept {
    return chunk;
}

void OutBuf::flushImpl() {
    Buffer buf;

    buf.xchg(buf_);
    out_->write(buf.data(), buf.used());
}

void OutBuf::finishImpl() {
    flush();
    out_ = nullptr;
}

void OutBuf::xchg(OutBuf& buf) noexcept {
    ::Std::xchg(buf_, buf.buf_);
    ::Std::xchg(out_, buf.out_);
    ::Std::xchg(chunk, buf.chunk);
}
