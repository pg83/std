#include "buffer.h"

#include <std/ios/buf.h>

#include <std/mem/bss.h>
#include <std/sys/crt.h>

#include <std/alg/bits.h>

#include <new>
#include <string.h>

using namespace Std;

Buffer::Header* Buffer::Header::null() noexcept {
    return (Header*)bss();
}

Buffer::Header* Buffer::Header::alloc(size_t len) {
    if (len) {
        return new (allocateMemory(sizeof(Header) + len)) Header(len);
    }

    return null();
}

void Buffer::Header::free(Header* ptr) noexcept {
    if (ptr == null()) {
    } else {
        freeMemory(ptr);
    }
}

Buffer::~Buffer() {
    Header::free(header());
}

Buffer::Buffer() noexcept
    : Buffer(0)
{
}

Buffer::Buffer(size_t len)
    : data_(Header::alloc(len) + 1)
{
}

Buffer::Buffer(const Buffer& buf)
    : Buffer(buf.data(), buf.used())
{
}

Buffer::Buffer(const void* ptr, size_t len)
    : Buffer()
{
    append(ptr, len);
}

Buffer::Buffer(Buffer&& buf) noexcept
    : Buffer()
{
    buf.xchg(*this);
}

void Buffer::shrinkToFit() {
    Buffer(*this).xchg(*this);
}

void Buffer::grow(size_t size) {
    if (size > capacity()) {
        Buffer buf(clp2(size + sizeof(Header)) - sizeof(Header));

        buf.appendUnsafe(data(), used());
        buf.xchg(*this);
    }
}

void Buffer::append(const void* ptr, size_t len) {
    grow(used() + len);
    appendUnsafe(ptr, len);
}

void Buffer::appendUnsafe(const void* ptr, size_t len) {
    auto cur = (char*)data() + used();

    if (len == 1) {
        *cur = *(const char*)ptr;
        header()->used += 1;
    } else {
        memcpy(cur, ptr, len);
        header()->used += len;
    }
}

template <>
void Std::output<OutBuf, Buffer>(OutBuf& out, const Buffer& buf) {
    out.write(buf.data(), buf.used());
}
