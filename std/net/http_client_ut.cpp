#include "http_client.h"
#include "tcp_socket.h"

#include <std/tst/ut.h>
#include <std/ios/out.h>
#include <std/ios/sys.h>
#include <std/sys/crt.h>
#include <std/tst/args.h>
#include <std/thr/coro.h>
#include <std/alg/defer.h>
#include <std/dbg/insist.h>
#include <std/lib/buffer.h>
#include <std/ios/in_buf.h>
#include <std/map/map.h>
#include <std/ios/in_mem.h>
#include <std/str/builder.h>
#include <std/mem/obj_pool.h>
#include <std/thr/channel.h>
#include <std/thr/wait_group.h>
#include <std/ios/stream_tcp.h>
#include <std/thr/coro_config.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace stl;

namespace {
    HttpClientResponse* parseResponse(ObjPool* pool, MemoryInput* in) {
        auto out = createNullOutput(pool);
        auto req = HttpClientRequest::create(pool, in, out);

        req->endHeaders();

        return req->response();
    }
}

STD_TEST_SUITE(HttpClient) {
    STD_TEST(StatusAndReason) {
        const char* data =
            "HTTP/1.1 200 OK\r\n"
            "\r\n";

        MemoryInput in(data, StringView(data).length());
        auto pool = ObjPool::fromMemory();
        auto cli = parseResponse(pool.mutPtr(), &in);

        STD_INSIST(cli->status() == 200);
        STD_INSIST(cli->reason() == StringView("OK"));
    }

    STD_TEST(Status404) {
        const char* data =
            "HTTP/1.1 404 Not Found\r\n"
            "\r\n";

        MemoryInput in(data, StringView(data).length());
        auto pool = ObjPool::fromMemory();
        auto cli = parseResponse(pool.mutPtr(), &in);

        STD_INSIST(cli->status() == 404);
        STD_INSIST(cli->reason() == StringView("Not Found"));
    }

    STD_TEST(HeaderLookup) {
        const char* data =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "X-Custom: hello\r\n"
            "\r\n";

        MemoryInput in(data, StringView(data).length());
        auto pool = ObjPool::fromMemory();
        auto cli = parseResponse(pool.mutPtr(), &in);

        auto ct = cli->header(StringView("content-type"));

        STD_INSIST(ct != nullptr);
        STD_INSIST(*ct == StringView("text/html"));

        auto xc = cli->header(StringView("x-custom"));

        STD_INSIST(xc != nullptr);
        STD_INSIST(*xc == StringView("hello"));

        STD_INSIST(cli->header(StringView("missing")) == nullptr);
    }

    STD_TEST(ContentLengthBody) {
        const char* data =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 5\r\n"
            "\r\n"
            "hello";

        MemoryInput in(data, StringView(data).length());
        auto pool = ObjPool::fromMemory();
        auto cli = parseResponse(pool.mutPtr(), &in);

        Buffer body;

        cli->body()->readAll(body);

        STD_INSIST(StringView(body) == StringView("hello"));
    }

    STD_TEST(ChunkedBody) {
        const char* data =
            "HTTP/1.1 200 OK\r\n"
            "Transfer-Encoding: chunked\r\n"
            "\r\n"
            "5\r\n"
            "hello\r\n"
            "6\r\n"
            " world\r\n"
            "0\r\n"
            "\r\n";

        MemoryInput in(data, StringView(data).length());
        auto pool = ObjPool::fromMemory();
        auto cli = parseResponse(pool.mutPtr(), &in);

        Buffer body;

        cli->body()->readAll(body);

        STD_INSIST(StringView(body) == StringView("hello world"));
    }

    STD_TEST(NoBody) {
        const char* data =
            "HTTP/1.1 204 No Content\r\n"
            "\r\n";

        MemoryInput in(data, StringView(data).length());
        auto pool = ObjPool::fromMemory();
        auto cli = parseResponse(pool.mutPtr(), &in);

        Buffer body;

        cli->body()->readAll(body);

        STD_INSIST(body.empty());
    }

    STD_TEST(BodyDoesNotOverread) {
        const char* data =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 3\r\n"
            "\r\n"
            "abcEXTRA";

        MemoryInput in(data, StringView(data).length());
        auto pool = ObjPool::fromMemory();
        auto cli = parseResponse(pool.mutPtr(), &in);

        Buffer body;

        cli->body()->readAll(body);

        STD_INSIST(StringView(body) == StringView("abc"));
    }

    STD_TEST(MultipleHeaders) {
        const char* data =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 0\r\n"
            "Connection: keep-alive\r\n"
            "Server: test\r\n"
            "\r\n";

        MemoryInput in(data, StringView(data).length());
        auto pool = ObjPool::fromMemory();
        auto cli = parseResponse(pool.mutPtr(), &in);

        STD_INSIST(cli->status() == 200);
        STD_INSIST(*cli->header(StringView("content-length")) == StringView("0"));
        STD_INSIST(*cli->header(StringView("connection")) == StringView("keep-alive"));
        STD_INSIST(*cli->header(StringView("server")) == StringView("test"));
    }

    STD_TEST(SequentialResponses) {
        const char* data =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 2\r\n"
            "\r\n"
            "ok"
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 3\r\n"
            "\r\n"
            "err";

        MemoryInput in(data, StringView(data).length());
        auto pool = ObjPool::fromMemory();

        auto r1 = parseResponse(pool.mutPtr(), &in);

        STD_INSIST(r1->status() == 200);

        Buffer b1;

        r1->body()->readAll(b1);

        STD_INSIST(StringView(b1) == StringView("ok"));

        auto r2 = parseResponse(pool.mutPtr(), &in);

        STD_INSIST(r2->status() == 404);

        Buffer b2;

        r2->body()->readAll(b2);

        STD_INSIST(StringView(b2) == StringView("err"));
    }
}
