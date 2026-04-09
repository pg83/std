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

    STD_TEST(_LoadTest) {
        auto& a = _ctx.args();

        // url=http://host:port/path coros=100 threads=4 duration=10 payload=0
        auto urlArg = a.find(StringView("url"));

        if (!urlArg) {
            return;
        }

        StringView url = *urlArg;

        // strip http://
        if (url.startsWith(StringView("http://"))) {
            url = StringView(url.data() + 7, url.length() - 7);
        }

        // split host:port and path
        StringView hostPort, path;

        if (!url.split('/', hostPort, path)) {
            hostPort = url;
            path = StringView("/");
        } else {
            // path needs leading /
            // build /path
        }

        StringView host, portStr;
        u16 port = 80;

        if (hostPort.split(':', host, portStr)) {
            port = (u16)portStr.stou();
        } else {
            host = hostPort;
        }

        u32 numCoros = 100;
        u32 numThreads = 4;
        u32 durationSec = 10;
        u32 payloadLen = 0;

        if (auto v = a.find(StringView("coros"))) {
            numCoros = (u32)v->stou();
        }

        if (auto v = a.find(StringView("threads"))) {
            numThreads = (u32)v->stou();
        }

        if (auto v = a.find(StringView("duration"))) {
            durationSec = (u32)v->stou();
        }

        if (auto v = a.find(StringView("payload"))) {
            payloadLen = (u32)v->stou();
        }

        auto pool = ObjPool::fromMemory();

        sockaddr_in addr{};

        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(0x7f000001);

        // build payload header value
        Buffer payloadBuf;

        for (u32 i = 0; i < payloadLen; ++i) {
            u8 x = 'X';
            payloadBuf.append(&x, 1);
        }

        StringView payloadVal(payloadBuf);

        // build path with leading /
        StringBuilder pathBuf;

        pathBuf << StringView(u8"/") << path;

        StringView fullPath = pool.mutPtr()->intern(StringView(pathBuf));

        sysE << StringView(u8"host: ") << host << endL;
        sysE << StringView(u8"port: ") << (u64)port << endL;
        sysE << StringView(u8"path: ") << fullPath << endL;
        sysE << StringView(u8"threads: ") << (u64)numThreads << endL;
        sysE << StringView(u8"coros: ") << (u64)numCoros << endL;
        sysE << StringView(u8"duration: ") << (u64)durationSec << StringView(u8"s") << endL;
        sysE << StringView(u8"payload: ") << (u64)payloadLen << endL;

        struct ReqRecord {
            u32 status;
            u64 elapsedUs;
        };

        auto exec = CoroExecutor::create(pool.mutPtr(), CoroConfig(numThreads));

        Channel ch(exec, numCoros * 128);
        WaitGroup wg(exec);

        u64 startUs = monotonicNowUs();
        u64 deadlineUs = startUs + (u64)durationSec * 1000000;

        // aggregator coroutine
        u64 totalReqs = 0;
        Map<u64, u64> statuses(pool.mutPtr());

        exec->spawn([&] {
            void* batch[256];

            for (;;) {
                size_t n = ch.dequeue(batch, 256);

                if (n == 0) {
                    break;
                }

                for (size_t j = 0; j < n; ++j) {
                    auto rec = (ReqRecord*)batch[j];

                    ++statuses[(u64)rec->status];
                    ++totalReqs;
                }
            }
        });

        // worker coroutines
        for (u32 i = 0; i < numCoros; ++i) {
            auto coroPool = ObjPool::create(pool.mutPtr());

            wg.inc();

            exec->spawn([&, coroPool] {
                STD_DEFER {
                    wg.done();
                };

                TcpSocket sock(exec);

                STD_INSIST(sock.socket(AF_INET, SOCK_STREAM, 0) == 0);

                STD_DEFER {
                    sock.close();
                };

                STD_INSIST(sock.connectInf((const sockaddr*)&addr, sizeof(addr)) == 0);

                TcpStream stream(sock);
                InBuf in(stream);

                Buffer body;

                while (monotonicNowUs() < deadlineUs) {
                    auto rpool = ObjPool::fromMemory();
                    auto req = HttpClientRequest::create(rpool.mutPtr(), &in, &stream);

                    req->setPath(fullPath);
                    req->addHeader(StringView("Host"), host);
                    req->addHeader(StringView("Connection"), StringView("keep-alive"));

                    if (payloadLen > 0) {
                        req->addHeader(StringView("X-Payload"), payloadVal);
                    }

                    u64 t0 = monotonicNowUs();

                    req->endHeaders();

                    auto resp = req->response();
                    u32 st = resp->status();

                    if (st == 0) {
                        break;
                    }

                    resp->body()->readAll(body);

                    u64 t1 = monotonicNowUs();

                    auto rec = coroPool->make<ReqRecord>();

                    rec->status = st;
                    rec->elapsedUs = t1 - t0;

                    ch.enqueue(rec);
                }
            });
        }

        // wait for all workers, close channel, join executor
        exec->spawn([&] {
            wg.wait();
            ch.close();
        });

        exec->join();

        u64 elapsedUs = monotonicNowUs() - startUs;
        double elapsedSec = (double)elapsedUs / 1000000.0;
        double rps = (double)totalReqs / elapsedSec;

        sysE << StringView(u8"requests: ") << totalReqs
             << StringView(u8", elapsed: ") << elapsedSec
             << StringView(u8"s, rps: ") << rps
             << endL;

        statuses.visit([](u64 code, u64& count) {
            sysE << StringView(u8"  ") << code << StringView(u8": ") << count << endL;
        });
    }
}
