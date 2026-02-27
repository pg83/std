#include "in_buf.h"

#include "input.h"

#include <std/alg/xchg.h>

using namespace Std;

InBuf::~InBuf() {
}

InBuf::InBuf(Input& in) noexcept
    : InBuf(in, in.hint(1 << 14))
{
}

InBuf::InBuf(Input& in, size_t chunkSize) noexcept
    : in_(&in)
    , buf(chunkSize)
    , pos(0)
{
    // buf.setCapacity(chunkSize);
}

InBuf::InBuf() noexcept
    : in_(nullptr)
    , pos(0)
{
}

size_t InBuf::hintImpl() const noexcept {
    return buf.capacity();
}

size_t InBuf::nextImpl(const void** ptr) {
    if (pos >= buf.used()) {
        buf.seekAbsolute(in_->read(buf.mutData(), buf.capacity()));
        pos = 0;
    }

    return (*ptr = (const u8*)buf.data() + pos, buf.used() - pos);
}

void InBuf::commitImpl(size_t len) noexcept {
    pos += len;
}

void InBuf::xchg(InBuf& r) noexcept {
    ::Std::xchg(buf, r.buf);
    ::Std::xchg(in_, r.in_);
    ::Std::xchg(pos, r.pos);
}
