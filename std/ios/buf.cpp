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

OutBuf::OutBuf(Output& out) noexcept
    : out_(&out)
{
    buf_.grow(256);
}

OutBuf::OutBuf() noexcept
    : out_(nullptr)
{
}

void* OutBuf::imbueImpl(size_t len) {
    buf_.growDelta(len);

    return (void*)buf_.mutCurrent();
}

void OutBuf::bumpImpl(const void* ptr) noexcept {
    buf_.seekAbsolute(ptr);

    if (buf_.used() > 16 * 1024) {
        flush();
    }
}

void OutBuf::writeImpl(const void* ptr, size_t len) {
    if (auto maxb = min<size_t>(hint(), 16 * 1024); (buf_.used() + len) < maxb) {
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
    return out_->hint();
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
    ::Std::xchg(out_, buf.out_);
    buf_.xchg(buf.buf_);
}
