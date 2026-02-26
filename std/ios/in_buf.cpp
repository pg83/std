#include "in_buf.h"

#include "input.h"

#include <std/alg/xchg.h>

using namespace Std;

InBuf::~InBuf() {
}

InBuf::InBuf(Input& in) noexcept
    : InBuf(in, in.hint(1 << 16))
{
}

InBuf::InBuf(Input& in, size_t chunkSize) noexcept
    : in_(&in)
    , buf_(chunkSize)
    , pos(0)
{
    buf_.setCapacity(chunkSize);
}

InBuf::InBuf() noexcept
    : in_(nullptr)
    , pos(0)
{
}

size_t InBuf::hintImpl() const noexcept {
    return buf_.capacity();
}

size_t InBuf::nextImpl(const void** ptr) {
    if (pos >= buf_.used()) {
        buf_.seekAbsolute(in_->read(buf_.mutData(), buf_.capacity()));
        pos = 0;
    }

    auto available = buf_.used() - pos;

    *ptr = (const u8*)buf_.data() + pos;

    return available;
}

void InBuf::commitImpl(size_t len) noexcept {
    pos += len;
}

void InBuf::xchg(InBuf& buf) noexcept {
    ::Std::xchg(buf_, buf.buf_);
    ::Std::xchg(in_, buf.in_);
    ::Std::xchg(pos, buf.pos);
}
