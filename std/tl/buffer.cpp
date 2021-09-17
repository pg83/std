#include "buffer.h"

#include <std/os/bss.h>
#include <std/os/alloc.h>

#include <new>

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
    : Buffer(buf.data(), buf.size())
{
}

Buffer::Buffer(const void* data, size_t len)
    : Buffer()
{
    append(data, len);
}

Buffer::Buffer(Buffer&& buf) noexcept
    : Buffer()
{
    buf.swap(*this);
}

void Buffer::shrinkToFit() {
    Buffer(*this).swap(*this);
}

void Buffer::grow(size_t size) {
}

void Buffer::append(const void* data, size_t len) {
}
