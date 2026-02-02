#include "buffer.h"

#include <std/ios/buf.h>
#include <std/mem/bss.h>
#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/alg/bits.h>
#include <std/alg/minmax.h>
#include <std/dbg/assert.h>

using namespace Std;

static_assert(sizeof(Buffer) == sizeof(void*));

namespace {
    static inline auto nullHeader() noexcept {
        return (Buffer::Header*)bss();
    }

    static inline auto allocHeader(size_t len) {
        if (len) {
            auto flen = max<size_t>(clp2(len + sizeof(Buffer::Header)), 256);
            auto blen = flen - sizeof(Buffer::Header);

            return new (allocateMemory(flen)) Buffer::Header({
                .used = 0,
                .size = blen,
            });
        }

        return nullHeader();
    }

    static inline void freeHeader(Buffer::Header* ptr) noexcept {
        if (ptr == nullHeader()) {
        } else {
            freeMemory(ptr);
        }
    }
}

Buffer::~Buffer() noexcept {
    freeHeader(header());
}

Buffer::Buffer() noexcept
    : Buffer(0)
{
}

Buffer::Buffer(size_t len)
    : data_(allocHeader(len) + 1)
{
}

Buffer::Buffer(const Buffer& buf)
    : Buffer(buf.data(), buf.used())
{
}

Buffer::Buffer(const void* ptr, size_t len)
    : Buffer(len)
{
    appendUnsafe(ptr, len);
}

Buffer::Buffer(const StringView& v)
    : Buffer(v.data(), v.length())
{
}

Buffer::Buffer(Buffer&& buf) noexcept
    : Buffer()
{
    buf.xchg(*this);
}

void Buffer::shrinkToFit() {
    Buffer(*this).xchg(*this);
}

void Buffer::seekAbsolute(size_t pos) noexcept {
    if (header()->used != pos) {
        STD_ASSERT(pos <= capacity());
        header()->used = pos;
    }
}

void Buffer::grow(size_t size) {
    if (size > capacity()) {
        Buffer buf(size);

        buf.appendUnsafe(data(), used());
        buf.xchg(*this);
    }
}

void Buffer::append(const void* ptr, size_t len) {
    growDelta(len);
    appendUnsafe(ptr, len);
}

void Buffer::appendUnsafe(const void* ptr, size_t len) {
    auto cur = (u8*)mutCurrent();

    if (len == 1) {
        *cur = *(const u8*)ptr;
        header()->used += 1;
    } else if (len) {
        memCpy(cur, ptr, len);
        header()->used += len;
    }
}

template <>
void Std::output<ZeroCopyOutput, Buffer>(ZeroCopyOutput& out, const Buffer& buf) {
    out.write(buf.data(), buf.used());
}
