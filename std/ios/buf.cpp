#include "buf.h"
#include "manip.h"
#include "output.h"
#include "unbound.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/alg/xchg.h>
#include <std/alg/minmax.h>
#include <std/alg/advance.h>

#include <sys/uio.h>

using namespace Std;
using namespace Std::Manip;

OutBuf::~OutBuf() {
    if (out_) {
        finish();
    }
}

OutBuf::OutBuf(Output& out, size_t chunkSize) noexcept
    : out_(&out)
    , chunk(min(chunkSize, out_->hint()))
{
}

OutBuf::OutBuf() noexcept
    : out_(nullptr)
    , chunk(0)
{
}

void* OutBuf::imbueImpl(size_t len) {
    buf_.growDelta(len);

    return (void*)buf_.mutCurrent();
}

void OutBuf::bumpImpl(const void* ptr) noexcept {
    buf_.seekAbsolute(ptr);

    if (buf_.used() >= chunk) {
        Buffer buf;
        buf.xchg(buf_);
        writeDirect(buf.data(), buf.length());
    }
}

void OutBuf::writeDirect(const void* ptr, size_t len) {
    const auto cnt = len - len % chunk;

    out_->write(ptr, cnt);
    buf_.append(advancePtr(ptr, cnt), len - cnt);
}

void OutBuf::writeSlow(const void* ptr, size_t len) {
    Buffer buf;

    buf.xchg(buf_);

    const auto cnt = chunk - buf.used();

    iovec parts[] = {
        {
            .iov_base = (void*)buf.data(),
            .iov_len = buf.used(),
        },
        {
            .iov_base = (void*)ptr,
            .iov_len = cnt,
        },
    };

    out_->writeV(parts, 2);

    writeDirect(advancePtr(ptr, cnt), len - cnt);
}

void OutBuf::writeImpl(const void* ptr, size_t len) {
    if (buf_.used() + len < chunk) {
        buf_.append(ptr, len);
    } else if (buf_.used()) {
        writeSlow(ptr, len);
    } else {
        writeDirect(ptr, len);
    }
}

size_t OutBuf::hintImpl() const noexcept {
    return chunk;
}

void OutBuf::flushImpl() {
    Buffer buf;

    buf.xchg(buf_);
    out_->write(buf.data(), buf.used());
    buf.reset();
    buf.xchg(buf_);
}

void OutBuf::finishImpl() {
    flush();
    out_ = nullptr;
}

void OutBuf::xchg(OutBuf& buf) noexcept {
    ::Std::xchg(buf_, buf.buf_);
    ::Std::xchg(out_, buf.out_);
    ::Std::xchg(chunk, buf.chunk);
}
