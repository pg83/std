#include "http_io.h"

#include <std/tst/ut.h>
#include <std/dbg/insist.h>
#include <std/lib/buffer.h>
#include <std/ios/in_buf.h>
#include <std/ios/in_mem.h>
#include <std/str/builder.h>
#include <std/mem/obj_pool.h>

#include <string.h>

using namespace stl;

namespace {
    void drain(ZeroCopyInput* in, Buffer& out) {
        in->readAll(out);
    }
}

STD_TEST_SUITE(LimitedInput) {
    STD_TEST(BasicRead) {
        const char* data = "hello world";
        MemoryInput inner(data, 11);
        auto pool = ObjPool::fromMemory();
        auto in = createLimitedInput(pool.mutPtr(), &inner, 5);

        Buffer out;
        drain(in, out);

        STD_INSIST(out == StringView(u8"hello"));
    }

    STD_TEST(ExactLimit) {
        const char* data = "hello";
        MemoryInput inner(data, 5);
        auto pool = ObjPool::fromMemory();
        auto in = createLimitedInput(pool.mutPtr(), &inner, 5);

        Buffer out;
        drain(in, out);

        STD_INSIST(out == StringView(u8"hello"));
    }

    STD_TEST(ZeroLimit) {
        const char* data = "hello";
        MemoryInput inner(data, 5);
        auto pool = ObjPool::fromMemory();
        auto in = createLimitedInput(pool.mutPtr(), &inner, 0);

        Buffer out;
        drain(in, out);

        STD_INSIST(out.empty());
    }

    STD_TEST(ReadViaReadImpl) {
        const char* data = "hello world";
        MemoryInput inner(data, 11);
        auto pool = ObjPool::fromMemory();
        auto in = createLimitedInput(pool.mutPtr(), &inner, 5);

        u8 buf[10] = {};
        size_t n = in->read(buf, sizeof(buf));

        STD_INSIST(n == 5);
        STD_INSIST(memcmp(buf, "hello", 5) == 0);
    }

    STD_TEST(EOFAfterLimit) {
        const char* data = "hello world";
        MemoryInput inner(data, 11);
        auto pool = ObjPool::fromMemory();
        auto in = createLimitedInput(pool.mutPtr(), &inner, 5);

        u8 buf[10] = {};
        in->read(buf, 5);

        size_t n = in->read(buf, 5);

        STD_INSIST(n == 0);
    }

    STD_TEST(InnerAdvancedByLimit) {
        const char* data = "helloworld";
        MemoryInput inner(data, 10);
        auto pool = ObjPool::fromMemory();
        auto in = createLimitedInput(pool.mutPtr(), &inner, 5);

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
        auto in = createChunkedInput(pool.mutPtr(), &inner);

        Buffer out;
        drain(in, out);

        STD_INSIST(out == StringView(u8"hello"));
    }

    STD_TEST(MultipleChunks) {
        const char* data = "5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n";
        MemoryInput inner(data, strlen(data));
        auto pool = ObjPool::fromMemory();
        auto in = createChunkedInput(pool.mutPtr(), &inner);

        Buffer out;
        drain(in, out);

        STD_INSIST(out == StringView(u8"hello world"));
    }

    STD_TEST(EmptyBody) {
        const char* data = "0\r\n\r\n";
        MemoryInput inner(data, strlen(data));
        auto pool = ObjPool::fromMemory();
        auto in = createChunkedInput(pool.mutPtr(), &inner);

        Buffer out;
        drain(in, out);

        STD_INSIST(out.empty());
    }

    STD_TEST(ReadViaReadImpl) {
        const char* data = "5\r\nhello\r\n0\r\n\r\n";
        MemoryInput inner(data, strlen(data));
        auto pool = ObjPool::fromMemory();
        auto in = createChunkedInput(pool.mutPtr(), &inner);

        u8 buf[10] = {};
        size_t n = in->read(buf, sizeof(buf));

        STD_INSIST(n == 5);
        STD_INSIST(memcmp(buf, "hello", 5) == 0);
    }

    STD_TEST(SingleByteChunks) {
        const char* data = "1\r\na\r\n1\r\nb\r\n1\r\nc\r\n0\r\n\r\n";
        MemoryInput inner(data, strlen(data));
        auto pool = ObjPool::fromMemory();
        auto in = createChunkedInput(pool.mutPtr(), &inner);

        Buffer out;
        drain(in, out);

        STD_INSIST(out == StringView(u8"abc"));
    }

    STD_TEST(HexChunkSize) {
        // chunk size 16 = 0x10
        const char* data = "10\r\n0123456789abcdef\r\n0\r\n\r\n";
        MemoryInput inner(data, strlen(data));
        auto pool = ObjPool::fromMemory();
        auto in = createChunkedInput(pool.mutPtr(), &inner);

        Buffer out;
        drain(in, out);

        STD_INSIST(out == StringView(u8"0123456789abcdef"));
    }

    STD_TEST(EOFAfterTerminator) {
        const char* data = "3\r\nabc\r\n0\r\n\r\n";
        MemoryInput inner(data, strlen(data));
        auto pool = ObjPool::fromMemory();
        auto in = createChunkedInput(pool.mutPtr(), &inner);

        Buffer out;
        drain(in, out);

        // second drain should be empty
        Buffer out2;
        drain(in, out2);

        STD_INSIST(out == StringView(u8"abc"));
        STD_INSIST(out2.empty());
    }
}

STD_TEST_SUITE(LimitedOutputRoundtrip) {
    STD_TEST(SmallWrite) {
        auto pool = ObjPool::fromMemory();
        StringBuilder sb;
        auto out = createLimitedOutput(pool.mutPtr(), &sb, 5);

        out->write("hello", 5);

        MemoryInput mi(sb.data(), sb.used());
        auto in = createLimitedInput(pool.mutPtr(), &mi, 5);

        Buffer result;
        drain(in, result);

        STD_INSIST(result == StringView(u8"hello"));
    }

    STD_TEST(ExactLimit) {
        auto pool = ObjPool::fromMemory();
        StringBuilder sb;
        auto out = createLimitedOutput(pool.mutPtr(), &sb, 3);

        out->write("abc", 3);

        MemoryInput mi(sb.data(), sb.used());
        auto in = createLimitedInput(pool.mutPtr(), &mi, 3);

        Buffer result;
        drain(in, result);

        STD_INSIST(result == StringView(u8"abc"));
    }

    STD_TEST(MultipleWrites) {
        auto pool = ObjPool::fromMemory();
        StringBuilder sb;
        auto out = createLimitedOutput(pool.mutPtr(), &sb, 10);

        out->write("hello", 5);
        out->write("world", 5);

        MemoryInput mi(sb.data(), sb.used());
        auto in = createLimitedInput(pool.mutPtr(), &mi, 10);

        Buffer result;
        drain(in, result);

        STD_INSIST(result == StringView(u8"helloworld"));
    }

    STD_TEST(SingleByteWrites) {
        auto pool = ObjPool::fromMemory();
        StringBuilder sb;
        auto out = createLimitedOutput(pool.mutPtr(), &sb, 4);

        out->write("a", 1);
        out->write("b", 1);
        out->write("c", 1);
        out->write("d", 1);

        MemoryInput mi(sb.data(), sb.used());
        auto in = createLimitedInput(pool.mutPtr(), &mi, 4);

        Buffer result;
        drain(in, result);

        STD_INSIST(result == StringView(u8"abcd"));
    }
}

STD_TEST_SUITE(ChunkedOutputRoundtrip) {
    STD_TEST(SingleWrite) {
        auto pool = ObjPool::fromMemory();
        StringBuilder sb;
        auto out = createChunkedOutput(pool.mutPtr(), &sb);

        out->write("hello", 5);
        out->finish();

        MemoryInput mi(sb.data(), sb.used());
        InBuf ib(mi);
        auto in = createChunkedInput(pool.mutPtr(), &ib);

        Buffer result;
        drain(in, result);

        STD_INSIST(result == StringView(u8"hello"));
    }

    STD_TEST(MultipleWrites) {
        auto pool = ObjPool::fromMemory();
        StringBuilder sb;
        auto out = createChunkedOutput(pool.mutPtr(), &sb);

        out->write("hello", 5);
        out->write(" ", 1);
        out->write("world", 5);
        out->finish();

        MemoryInput mi(sb.data(), sb.used());
        InBuf ib(mi);
        auto in = createChunkedInput(pool.mutPtr(), &ib);

        Buffer result;
        drain(in, result);

        STD_INSIST(result == StringView(u8"hello world"));
    }

    STD_TEST(EmptyBody) {
        auto pool = ObjPool::fromMemory();
        StringBuilder sb;
        auto out = createChunkedOutput(pool.mutPtr(), &sb);

        out->finish();

        MemoryInput mi(sb.data(), sb.used());
        InBuf ib(mi);
        auto in = createChunkedInput(pool.mutPtr(), &ib);

        Buffer result;
        drain(in, result);

        STD_INSIST(result.empty());
    }

    STD_TEST(SingleByteWrites) {
        auto pool = ObjPool::fromMemory();
        StringBuilder sb;
        auto out = createChunkedOutput(pool.mutPtr(), &sb);

        out->write("a", 1);
        out->write("b", 1);
        out->write("c", 1);
        out->finish();

        MemoryInput mi(sb.data(), sb.used());
        InBuf ib(mi);
        auto in = createChunkedInput(pool.mutPtr(), &ib);

        Buffer result;
        drain(in, result);

        STD_INSIST(result == StringView(u8"abc"));
    }

    STD_TEST(LargeWrite) {
        auto pool = ObjPool::fromMemory();
        StringBuilder sb;
        auto out = createChunkedOutput(pool.mutPtr(), &sb);

        u8 data[1024];

        for (size_t i = 0; i < sizeof(data); i++) {
            data[i] = (u8)(i & 0xff);
        }

        out->write(data, sizeof(data));
        out->finish();

        MemoryInput mi(sb.data(), sb.used());
        InBuf ib(mi);
        auto in = createChunkedInput(pool.mutPtr(), &ib);

        Buffer result;
        drain(in, result);

        STD_INSIST(result.used() == sizeof(data));
        STD_INSIST(memcmp(result.data(), data, sizeof(data)) == 0);
    }

    STD_TEST(ManySmallChunks) {
        auto pool = ObjPool::fromMemory();
        StringBuilder sb;
        auto out = createChunkedOutput(pool.mutPtr(), &sb);

        for (int i = 0; i < 100; i++) {
            out->write("x", 1);
        }

        out->finish();

        MemoryInput mi(sb.data(), sb.used());
        InBuf ib(mi);
        auto in = createChunkedInput(pool.mutPtr(), &ib);

        Buffer result;
        drain(in, result);

        STD_INSIST(result.used() == 100);

        for (size_t i = 0; i < result.used(); i++) {
            STD_INSIST(((const u8*)result.data())[i] == 'x');
        }
    }

    STD_TEST(HexSizeRoundtrip) {
        auto pool = ObjPool::fromMemory();
        StringBuilder sb;
        auto out = createChunkedOutput(pool.mutPtr(), &sb);

        // 256 bytes - chunk size should be "100" in hex
        u8 data[256];

        for (size_t i = 0; i < sizeof(data); i++) {
            data[i] = (u8)(i & 0xff);
        }

        out->write(data, sizeof(data));
        out->finish();

        MemoryInput mi(sb.data(), sb.used());
        InBuf ib(mi);
        auto in = createChunkedInput(pool.mutPtr(), &ib);

        Buffer result;
        drain(in, result);

        STD_INSIST(result.used() == sizeof(data));
        STD_INSIST(memcmp(result.data(), data, sizeof(data)) == 0);
    }
}
