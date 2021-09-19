#include "buf.h"
#include "manip.h"
#include "output.h"
#include "unbound.h"

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

UnboundBuffer OutBuf::imbue(size_t len) {
    buf_.grow(buf_.used() + len);

    return buf_.used() + (u8*)buf_.data();
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
void Std::output<OutBuf, Flush>(OutBuf& out, Flush) {
    out.flush();
}

template <>
void Std::output<OutBuf, Finish>(OutBuf& out, Finish) {
    out.finish();
}

template <>
void Std::output<OutBuf, EndLine>(OutBuf& out, EndLine) {
    out << u8'\n';
}

// strings
template <>
void Std::output<OutBuf, const u8*>(OutBuf& out, const u8* str) {
    out.write(str, strLen(str));
}

// std types
template <>
void Std::output<OutBuf, u8>(OutBuf& out, u8 ch) {
    out.write(&ch, 1);
}

template <>
void Std::output<OutBuf, u16>(OutBuf& out, u16 v) {
    out << (u64)v;
}

template <>
void Std::output<OutBuf, u32>(OutBuf& out, u32 v) {
    out << (u64)v;
}

template <>
void Std::output<OutBuf, u64>(OutBuf& out, u64 v) {
    out.bump(out.imbue(24) << v);
}
