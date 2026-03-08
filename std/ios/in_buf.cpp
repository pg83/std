#include "in_buf.h"

#include "input.h"

#include <std/alg/xchg.h>

using namespace stl;

InBuf::~InBuf() {
}

InBuf::InBuf(Input& in)
    : InBuf(in, in.hint(1 << 14))
{
}

InBuf::InBuf(Input& in, size_t chunkSize)
    : in_(&in)
    , buf(chunkSize)
    , pos(0)
{
    // buf.setCapacity(chunkSize);
}

InBuf::InBuf()
    : in_(nullptr)
    , pos(0)
{
}

size_t InBuf::hintImpl() const {
    return buf.capacity();
}

size_t InBuf::nextImpl(const void** ptr) {
    if (pos >= buf.used()) {
        buf.seekAbsolute(in_->read(buf.mutData(), buf.capacity()));
        pos = 0;
    }

    return (*ptr = (const u8*)buf.data() + pos, buf.used() - pos);
}

void InBuf::commitImpl(size_t len) {
    pos += len;
}

void InBuf::xchg(InBuf& r) {
    ::stl::xchg(buf, r.buf);
    ::stl::xchg(in_, r.in_);
    ::stl::xchg(pos, r.pos);
}
