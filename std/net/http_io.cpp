#include "http_io.h"

#include <std/str/fmt.h>
#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/ios/in_zc.h>
#include <std/ios/output.h>
#include <std/lib/buffer.h>
#include <std/alg/minmax.h>
#include <std/dbg/insist.h>
#include <std/mem/obj_pool.h>

#include <sys/uio.h>

using namespace stl;

namespace {
    struct LimitedInput: public ZeroCopyInput {
        ZeroCopyInput* inner;
        size_t remaining;

        LimitedInput(ZeroCopyInput* inner, size_t limit);

        size_t readImpl(void* data, size_t len) override;
        size_t nextImpl(const void** chunk) override;
        void commitImpl(size_t len) noexcept override;
    };

    struct LimitedOutput: public Output {
        Output* inner;
        size_t remaining;

        LimitedOutput(Output* inner, size_t limit);

        size_t writeImpl(const void* data, size_t len) override;
        void flushImpl() override;
    };

    struct ChunkedOutput: public Output {
        Output* inner;

        ChunkedOutput(Output* inner);

        size_t writeImpl(const void* data, size_t len) override;
        void flushImpl() override;
        void finishImpl() override;
    };

    struct ChunkedInput: public ZeroCopyInput {
        ZeroCopyInput* inner;
        size_t chunkRemaining;
        bool eof;
        bool first;
        Buffer sizeBuf;

        ChunkedInput(ZeroCopyInput* inner);

        size_t nextImpl(const void** chunk) override;
        void commitImpl(size_t len) noexcept override;

        bool loadChunk();
    };
}

LimitedInput::LimitedInput(ZeroCopyInput* inner, size_t limit)
    : inner(inner)
    , remaining(limit)
{
}

size_t LimitedInput::readImpl(void* data, size_t len) {
    size_t n = inner->read(data, min(len, remaining));

    remaining -= n;

    return n;
}

size_t LimitedInput::nextImpl(const void** chunk) {
    if (!remaining) {
        return 0;
    }

    return min(inner->next(chunk), remaining);
}

void LimitedInput::commitImpl(size_t len) noexcept {
    inner->commit(len);
    remaining -= len;
}

ChunkedInput::ChunkedInput(ZeroCopyInput* inner)
    : inner(inner)
    , chunkRemaining(0)
    , eof(false)
    , first(true)
{
}

bool ChunkedInput::loadChunk() {
    if (!first) {
        char crlf[2];
        inner->read(crlf, 2);
    }

    first = false;
    sizeBuf.reset();
    inner->readLine(sizeBuf);
    chunkRemaining = StringView(sizeBuf).stripCr().stoh();

    if (!chunkRemaining) {
        char crlf[2];
        inner->read(crlf, 2);
        eof = true;
        return false;
    }

    return true;
}

size_t ChunkedInput::nextImpl(const void** chunk) {
    if (eof) {
        return 0;
    }

    if (!chunkRemaining && !loadChunk()) {
        return 0;
    }

    return min(inner->next(chunk), chunkRemaining);
}

void ChunkedInput::commitImpl(size_t len) noexcept {
    inner->commit(len);
    chunkRemaining -= len;
}

LimitedOutput::LimitedOutput(Output* inner, size_t limit)
    : inner(inner)
    , remaining(limit)
{
}

size_t LimitedOutput::writeImpl(const void* data, size_t len) {
    STD_INSIST(remaining > 0);

    size_t n = inner->write(data, min(len, remaining));

    remaining -= n;

    return n;
}

void LimitedOutput::flushImpl() {
    inner->flush();
}

ChunkedOutput::ChunkedOutput(Output* inner)
    : inner(inner)
{
}

size_t ChunkedOutput::writeImpl(const void* data, size_t len) {
    u8 buf[20];
    u8* e = (u8*)formatU64Base16(len, buf);

    *e++ = u8'\r';
    *e++ = u8'\n';

    iovec iov[3] = {
        {buf, (size_t)(e - buf)},
        {(void*)data, len},
        {(void*)u8"\r\n", 2},
    };

    inner->writeV(iov, 3);

    return len;
}

void ChunkedOutput::flushImpl() {
    inner->flush();
}

void ChunkedOutput::finishImpl() {
    inner->write(u8"0\r\n\r\n", 5);
}

ZeroCopyInput* stl::createLimitedInput(ObjPool* pool, ZeroCopyInput* inner, size_t limit) {
    return pool->make<LimitedInput>(inner, limit);
}

ZeroCopyInput* stl::createChunkedInput(ObjPool* pool, ZeroCopyInput* inner) {
    return pool->make<ChunkedInput>(inner);
}

Output* stl::createLimitedOutput(ObjPool* pool, Output* inner, size_t limit) {
    return pool->make<LimitedOutput>(inner, limit);
}

Output* stl::createChunkedOutput(ObjPool* pool, Output* inner) {
    return pool->make<ChunkedOutput>(inner);
}
