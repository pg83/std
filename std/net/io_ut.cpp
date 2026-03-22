#include "io.h"

#include <std/tst/ut.h>
#include <std/dbg/insist.h>
#include <std/lib/buffer.h>
#include <std/ios/in_mem.h>
#include <std/mem/obj_pool.h>

#include <cstring>

using namespace stl;

namespace {
    void drain(ZeroCopyInput* in, Buffer& out) {
        const void* chunk;
        size_t n;

        while ((n = in->next(&chunk))) {
            out.append((const u8*)chunk, n);
            in->commit(n);
        }
    }
}

STD_TEST_SUITE(LimitedInput) {
    STD_TEST(BasicRead) {
        const char* data = "hello world";
        MemoryInput inner(data, 11);
        auto pool = ObjPool::fromMemory();
        auto* in = createLimited(pool.mutPtr(), &inner, 5);

        Buffer out;
        drain(in, out);

        STD_INSIST(out == StringView(u8"hello"));
    }

    STD_TEST(ExactLimit) {
        const char* data = "hello";
        MemoryInput inner(data, 5);
        auto pool = ObjPool::fromMemory();
        auto* in = createLimited(pool.mutPtr(), &inner, 5);

        Buffer out;
        drain(in, out);

        STD_INSIST(out == StringView(u8"hello"));
    }

    STD_TEST(ZeroLimit) {
        const char* data = "hello";
        MemoryInput inner(data, 5);
        auto pool = ObjPool::fromMemory();
        auto* in = createLimited(pool.mutPtr(), &inner, 0);

        Buffer out;
        drain(in, out);

        STD_INSIST(out.empty());
    }

    STD_TEST(ReadViaReadImpl) {
        const char* data = "hello world";
        MemoryInput inner(data, 11);
        auto pool = ObjPool::fromMemory();
        auto* in = createLimited(pool.mutPtr(), &inner, 5);

        u8 buf[10] = {};
        size_t n = in->read(buf, sizeof(buf));

        STD_INSIST(n == 5);
        STD_INSIST(memcmp(buf, "hello", 5) == 0);
    }

    STD_TEST(EOFAfterLimit) {
        const char* data = "hello world";
        MemoryInput inner(data, 11);
        auto pool = ObjPool::fromMemory();
        auto* in = createLimited(pool.mutPtr(), &inner, 5);

        u8 buf[10] = {};
        in->read(buf, 5);

        size_t n = in->read(buf, 5);

        STD_INSIST(n == 0);
    }

    STD_TEST(InnerAdvancedByLimit) {
        const char* data = "helloworld";
        MemoryInput inner(data, 10);
        auto pool = ObjPool::fromMemory();
        auto* in = createLimited(pool.mutPtr(), &inner, 5);

        Buffer out;
        drain(in, out);

        // inner should now be positioned after "hello"
        const void* chunk;
        size_t n = inner.next(&chunk);

        STD_INSIST(n == 5);
        STD_INSIST(memcmp(chunk, "world", 5) == 0);
    }
}

STD_TEST_SUITE(ChunkedInput) {
    STD_TEST(SingleChunk) {
        const char* data = "5\r\nhello\r\n0\r\n\r\n";
        MemoryInput inner(data, strlen(data));
        auto pool = ObjPool::fromMemory();
        auto* in = createChunked(pool.mutPtr(), &inner);

        Buffer out;
        drain(in, out);

        STD_INSIST(out == StringView(u8"hello"));
    }

    STD_TEST(MultipleChunks) {
        const char* data = "5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n";
        MemoryInput inner(data, strlen(data));
        auto pool = ObjPool::fromMemory();
        auto* in = createChunked(pool.mutPtr(), &inner);

        Buffer out;
        drain(in, out);

        STD_INSIST(out == StringView(u8"hello world"));
    }

    STD_TEST(EmptyBody) {
        const char* data = "0\r\n\r\n";
        MemoryInput inner(data, strlen(data));
        auto pool = ObjPool::fromMemory();
        auto* in = createChunked(pool.mutPtr(), &inner);

        Buffer out;
        drain(in, out);

        STD_INSIST(out.empty());
    }

    STD_TEST(ReadViaReadImpl) {
        const char* data = "5\r\nhello\r\n0\r\n\r\n";
        MemoryInput inner(data, strlen(data));
        auto pool = ObjPool::fromMemory();
        auto* in = createChunked(pool.mutPtr(), &inner);

        u8 buf[10] = {};
        size_t n = in->read(buf, sizeof(buf));

        STD_INSIST(n == 5);
        STD_INSIST(memcmp(buf, "hello", 5) == 0);
    }

    STD_TEST(SingleByteChunks) {
        const char* data = "1\r\na\r\n1\r\nb\r\n1\r\nc\r\n0\r\n\r\n";
        MemoryInput inner(data, strlen(data));
        auto pool = ObjPool::fromMemory();
        auto* in = createChunked(pool.mutPtr(), &inner);

        Buffer out;
        drain(in, out);

        STD_INSIST(out == StringView(u8"abc"));
    }

    STD_TEST(HexChunkSize) {
        // chunk size 16 = 0x10
        const char* data = "10\r\n0123456789abcdef\r\n0\r\n\r\n";
        MemoryInput inner(data, strlen(data));
        auto pool = ObjPool::fromMemory();
        auto* in = createChunked(pool.mutPtr(), &inner);

        Buffer out;
        drain(in, out);

        STD_INSIST(out == StringView(u8"0123456789abcdef"));
    }

    STD_TEST(EOFAfterTerminator) {
        const char* data = "3\r\nabc\r\n0\r\n\r\n";
        MemoryInput inner(data, strlen(data));
        auto pool = ObjPool::fromMemory();
        auto* in = createChunked(pool.mutPtr(), &inner);

        Buffer out;
        drain(in, out);

        // second drain should be empty
        Buffer out2;
        drain(in, out2);

        STD_INSIST(out == StringView(u8"abc"));
        STD_INSIST(out2.empty());
    }
}
