#include "io.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/ios/in_zc.h>
#include <std/lib/buffer.h>
#include <std/alg/minmax.h>
#include <std/mem/obj_pool.h>

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

    struct ChunkedInput: public ZeroCopyInput {
        ZeroCopyInput* inner;
        size_t chunkRemaining;
        bool eof;
        bool first;
        Buffer sizeBuf;

        ChunkedInput(ZeroCopyInput* inner);

        size_t readImpl(void* data, size_t len) override;
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
    len = min(len, remaining);

    if (!len) {
        return 0;
    }

    size_t n = inner->read(data, len);

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
        eof = true;
        return false;
    }

    return true;
}

size_t ChunkedInput::readImpl(void* data, size_t len) {
    const void* chunk;

    len = min(next(&chunk), len);

    if (!len) {
        return 0;
    }

    memCpy(data, chunk, len);
    commit(len);

    return len;
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

ZeroCopyInput* stl::createLimited(ObjPool* pool, ZeroCopyInput* inner, size_t limit) {
    return pool->make<LimitedInput>(inner, limit);
}

ZeroCopyInput* stl::createChunked(ObjPool* pool, ZeroCopyInput* inner) {
    return pool->make<ChunkedInput>(inner);
}
