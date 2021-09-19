#include "buf.h"
#include "manip.h"
#include "output.h"

#include <std/str/fmt.h>
#include <std/sys/crt.h>

using namespace Std;
using namespace Std::Manip;

OutBuf::~OutBuf() {
    if (out_) {
        finish();
    }
}

OutBuf::OutBuf(Output& out) noexcept
    : out_(&out)
{
    buf_.grow(256);
}

OutBuf::OutBuf() noexcept
    : out_(nullptr)
{
}

size_t OutBuf::imbue(void** ptr) noexcept {
    *ptr = buf_.used() + (u8*)buf_.data();

    return buf_.left();
}

void OutBuf::write(const void* ptr, size_t len) {
    buf_.append(ptr, len);

    if (buf_.used() > 16 * 1024) {
        flush();
    }
}

void OutBuf::flush() {
    out_->write(buf_.data(), buf_.used());
    buf_.reset();
}

void OutBuf::finish() {
    flush();
    out_ = nullptr;
}

void OutBuf::xchg(OutBuf& buf) noexcept {
    ::Std::xchg(out_, buf.out_);
    buf_.xchg(buf.buf_);
}

// modifiers
template <>
void Std::output<Flush>(OutBuf& out, Flush) {
    out.flush();
}

template <>
void Std::output<Finish>(OutBuf& out, Finish) {
    out.finish();
}

template <>
void Std::output<EndLine>(OutBuf& out, EndLine) {
    out << u8'\n';
}

// strings
template <>
void Std::output<const u8*>(OutBuf& out, const u8* str) {
    out.write(str, strLen(str));
}

// std types
template <>
void Std::output<u8>(OutBuf& out, u8 ch) {
    out.write(&ch, 1);
}

template <>
void Std::output<u16>(OutBuf& out, u16 v) {
    out << (u64)v;
}

template <>
void Std::output<u32>(OutBuf& out, u32 v) {
    out << (u64)v;
}

template <>
void Std::output<u64>(OutBuf& out, u64 v) {
    void* ptr;

    if (auto len = out.imbue(&ptr); len >= 24) {
        out.bump(formatU64Base10(v, ptr));
    } else {
        char buf[24];

        out.write(buf, formatU64Base10(v, buf));
    }
}
