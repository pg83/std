#include "buf.h"
#include "manip.h"
#include "output.h"
#include "unbound.h"

#include <std/sys/crt.h>
#include <std/str/view.h>

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
}

void OutBuf::writeImpl(const void* ptr, size_t len) {
    if ((buf_.used() + len) < hint()) {
        buf_.append(ptr, len);
    } else {
        const StringView parts[] = {
            StringView((const u8*)buf_.data(), buf_.used()),
            StringView((const u8*)ptr, len),
        };

        out_->writeV(parts, 2);
        buf_.reset();
    }
}

size_t OutBuf::hintImpl() const noexcept {
    return out_->hint();
}

void OutBuf::flushImpl() {
    out_->write(buf_.data(), buf_.used());
    buf_.reset();
}

void OutBuf::finishImpl() {
    flush();
    out_ = nullptr;
}

void OutBuf::xchg(OutBuf& buf) noexcept {
    ::Std::xchg(out_, buf.out_);
    buf_.xchg(buf.buf_);
}
