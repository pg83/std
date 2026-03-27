#include "http_srv.h"
#include "http_client.h"
#include "ssl_socket.h"
#include "tcp_socket.h"

#include <std/sys/fd.h>
#include <std/tst/ut.h>
#include <std/ios/sys.h>
#include <std/sys/crt.h>
#include <std/tst/args.h>
#include <std/thr/coro.h>
#include <std/alg/defer.h>
#include <std/dbg/insist.h>
#include <std/alg/minmax.h>
#include <std/ios/in_buf.h>
#include <std/ios/out_buf.h>
#include <std/str/builder.h>
#include <std/mem/obj_pool.h>
#include <std/thr/wait_group.h>
#include <std/ios/stream_tcp.h>
#include <std/ios/in_fd_coro.h>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace stl;

namespace {
    sockaddr_in makeAddr(u16 port) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        return addr;
    }
}

STD_TEST_SUITE(HttpServerRequestParsing) {
    STD_TEST(PathNoQuery) {
        auto exec = CoroExecutor::create(4);

        struct Handler: HttpServe {
            Buffer path;
            Buffer query;

            void serve(HttpServerResponse& resp) override {
                path = Buffer(resp.request()->path());
                query = Buffer(resp.request()->query());

                resp.addHeader(StringView("Content-Length"), StringView("0"));
                resp.endHeaders();
            }
        } handler;

        auto addr = makeAddr(17670);
        auto pool = ObjPool::fromMemory();
        WaitGroup wg(exec.mutPtr());
        auto* ctl = serve(pool.mutPtr(), {
                                             .handler = &handler,
                                             .exec = exec.mutPtr(),
                                             .addr = (const sockaddr*)&addr,
                                             .addrLen = sizeof(addr),
                                             .wg = &wg,
                                         });

        exec->spawn([&] {
            TcpSocket cli(exec.mutPtr());
            auto caddr = makeAddr(17670);
            STD_INSIST(cli.connectInf((const sockaddr*)&caddr, sizeof(caddr)) == 0);
            STD_DEFER {
                cli.close();
            };

            TcpStream stream(cli);
            const char* req = "GET /foo/bar HTTP/1.0\r\n\r\n";
            stream.write(req, ::strlen(req));

            char buf[256] = {};
            size_t n = 0;
            cli.readInf(&n, buf, sizeof(buf) - 1);

            ctl->stop();
        });

        exec->spawn([&] {
            wg.wait();
        });
        exec->join();

        STD_INSIST(StringView(handler.path) == StringView("/foo/bar"));
        STD_INSIST(StringView(handler.query).empty());
    }

    STD_TEST(PathWithQuery) {
        auto exec = CoroExecutor::create(4);

        struct Handler: HttpServe {
            Buffer path;
            Buffer query;

            void serve(HttpServerResponse& resp) override {
                path = Buffer(resp.request()->path());
                query = Buffer(resp.request()->query());

                resp.addHeader(StringView("Content-Length"), StringView("0"));
                resp.endHeaders();
            }
        } handler;

        auto addr = makeAddr(17671);
        auto pool = ObjPool::fromMemory();
        WaitGroup wg(exec.mutPtr());
        auto* ctl = serve(pool.mutPtr(), {
                                             .handler = &handler,
                                             .exec = exec.mutPtr(),
                                             .addr = (const sockaddr*)&addr,
                                             .addrLen = sizeof(addr),
                                             .wg = &wg,
                                         });

        exec->spawn([&] {
            TcpSocket cli(exec.mutPtr());
            auto caddr = makeAddr(17671);
            STD_INSIST(cli.connectInf((const sockaddr*)&caddr, sizeof(caddr)) == 0);
            STD_DEFER {
                cli.close();
            };

            TcpStream stream(cli);
            const char* req = "GET /search?q=hello&page=2 HTTP/1.0\r\n\r\n";
            stream.write(req, ::strlen(req));

            char buf[256] = {};
            size_t n = 0;
            cli.readInf(&n, buf, sizeof(buf) - 1);

            ctl->stop();
        });

        exec->spawn([&] {
            wg.wait();
        });
        exec->join();

        STD_INSIST(StringView(handler.path) == StringView("/search"));
        STD_INSIST(StringView(handler.query) == StringView("q=hello&page=2"));
    }
}

STD_TEST_SUITE(HttpServer) {
    STD_TEST(GetRequest) {
        auto exec = CoroExecutor::create(4);

        struct Handler: HttpServe {
            void serve(HttpServerResponse& resp) override {
                resp.addHeader(StringView("Content-Length"), StringView("5"));
                resp.endHeaders();
                resp.out()->write("hello", 5);
            }
        } handler;

        auto addr = makeAddr(17661);
        auto pool = ObjPool::fromMemory();
        WaitGroup wg(exec.mutPtr());
        auto* ctl = serve(pool.mutPtr(), {
                                             .handler = &handler,
                                             .exec = exec.mutPtr(),
                                             .addr = (const sockaddr*)&addr,
                                             .addrLen = sizeof(addr),
                                             .wg = &wg,
                                         });

        u32 respStatus = 0;
        Buffer respBody;

        exec->spawn([&] {
            TcpSocket cli(exec.mutPtr());
            auto caddr = makeAddr(17661);
            STD_INSIST(cli.connectInf((const sockaddr*)&caddr, sizeof(caddr)) == 0);
            STD_DEFER {
                cli.close();
            };

            TcpStream stream(cli);
            const char* req =
                "GET /index.html HTTP/1.0\r\n"
                "Host: localhost\r\n"
                "\r\n";
            stream.write(req, ::strlen(req));

            InBuf in(stream);
            auto pool = ObjPool::fromMemory();
            auto* resp = HttpClient::create(pool.mutPtr(), &in);

            respStatus = resp->status();
            resp->body()->readAll(respBody);

            ctl->stop();
        });

        exec->spawn([&] {
            wg.wait();
        });

        exec->join();

        STD_INSIST(respStatus == 200);
        STD_INSIST(StringView(respBody) == StringView("hello"));
    }

    STD_TEST(KeepAliveChunked) {
        auto exec = CoroExecutor::create(4);

        struct Handler: HttpServe {
            void serve(HttpServerResponse& resp) override {
                resp.addHeader(StringView("Content-Length"), StringView("2"));
                resp.addHeader(StringView("Connection"), StringView("keep-alive"));
                resp.endHeaders();
                resp.out()->write("ok", 2);
            }
        } handler;

        auto addr = makeAddr(17663);
        auto pool = ObjPool::fromMemory();
        WaitGroup wg(exec.mutPtr());
        auto* ctl = serve(pool.mutPtr(), {
                                             .handler = &handler,
                                             .exec = exec.mutPtr(),
                                             .addr = (const sockaddr*)&addr,
                                             .addrLen = sizeof(addr),
                                             .wg = &wg,
                                         });

        Buffer body1;
        Buffer body2;

        exec->spawn([&] {
            TcpSocket cli(exec.mutPtr());
            auto caddr = makeAddr(17663);
            STD_INSIST(cli.connectInf((const sockaddr*)&caddr, sizeof(caddr)) == 0);
            STD_DEFER {
                cli.close();
            };

            TcpStream stream(cli);
            InBuf in(stream);
            auto pool = ObjPool::fromMemory();

            const char* req1 =
                "POST /a HTTP/1.1\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n"
                "5\r\n"
                "hello\r\n"
                "0\r\n"
                "\r\n";
            stream.write(req1, ::strlen(req1));

            auto* resp1 = HttpClient::create(pool.mutPtr(), &in);
            STD_INSIST(resp1->status() == 200);
            resp1->body()->readAll(body1);

            const char* req2 =
                "POST /b HTTP/1.1\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n"
                "5\r\n"
                "world\r\n"
                "0\r\n"
                "\r\n";
            stream.write(req2, ::strlen(req2));

            auto* resp2 = HttpClient::create(pool.mutPtr(), &in);
            STD_INSIST(resp2->status() == 200);
            resp2->body()->readAll(body2);

            ctl->stop();
        });

        exec->spawn([&] {
            wg.wait();
        });

        exec->join();

        STD_INSIST(StringView(body1) == StringView("ok"));
        STD_INSIST(StringView(body2) == StringView("ok"));
    }

    STD_TEST(KeepAlive) {
        auto exec = CoroExecutor::create(4);

        struct Handler: HttpServe {
            void serve(HttpServerResponse& resp) override {
                resp.addHeader(StringView("Content-Length"), StringView("2"));
                resp.addHeader(StringView("Connection"), StringView("keep-alive"));
                resp.endHeaders();
                resp.out()->write("ok", 2);
            }
        } handler;

        auto addr = makeAddr(17662);
        auto pool = ObjPool::fromMemory();
        WaitGroup wg(exec.mutPtr());
        auto* ctl = serve(pool.mutPtr(), {
                                             .handler = &handler,
                                             .exec = exec.mutPtr(),
                                             .addr = (const sockaddr*)&addr,
                                             .addrLen = sizeof(addr),
                                             .wg = &wg,
                                         });

        Buffer body1;
        Buffer body2;

        exec->spawn([&] {
            TcpSocket cli(exec.mutPtr());
            auto caddr = makeAddr(17662);
            STD_INSIST(cli.connectInf((const sockaddr*)&caddr, sizeof(caddr)) == 0);
            STD_DEFER {
                cli.close();
            };

            TcpStream stream(cli);
            InBuf in(stream);
            auto pool = ObjPool::fromMemory();

            const char* req1 =
                "GET /a HTTP/1.1\r\n"
                "Content-Length: 0\r\n"
                "\r\n";
            stream.write(req1, ::strlen(req1));

            auto* resp1 = HttpClient::create(pool.mutPtr(), &in);
            STD_INSIST(resp1->status() == 200);
            resp1->body()->readAll(body1);

            const char* req2 =
                "GET /b HTTP/1.1\r\n"
                "Content-Length: 0\r\n"
                "\r\n";
            stream.write(req2, ::strlen(req2));

            auto* resp2 = HttpClient::create(pool.mutPtr(), &in);
            STD_INSIST(resp2->status() == 200);
            resp2->body()->readAll(body2);

            ctl->stop();
        });

        exec->spawn([&] {
            wg.wait();
        });

        exec->join();

        STD_INSIST(StringView(body1) == StringView("ok"));
        STD_INSIST(StringView(body2) == StringView("ok"));
    }
}

namespace {
    const char testCert[] =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIBfzCCASWgAwIBAgIUUQjugcnFUjBED4h+YaPFRr7pMlowCgYIKoZIzj0EAwIw\n"
        "FDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MDMyMzE0MjI1MFoYDzIxMjYwMjI3\n"
        "MTQyMjUwWjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwWTATBgcqhkjOPQIBBggqhkjO\n"
        "PQMBBwNCAARfsZNKls055081/xImV4diaUrCimYW2k0m7Rhq/B5xBjWP5ETfBy3X\n"
        "aoUXGA02bImZaTSTLd43TWTCB5IbRkngo1MwUTAdBgNVHQ4EFgQURCfdIwDIDcdE\n"
        "hrObYyq2RJsofgswHwYDVR0jBBgwFoAURCfdIwDIDcdEhrObYyq2RJsofgswDwYD\n"
        "VR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAgNIADBFAiEAiISrnWrLf3ZHEnIfmPcM\n"
        "XMJDSo3hPE486n7a40YQQw8CIEICeaFmiGyD2MvrpAe+S6k80bGFOJeWDOzwJmP7\n"
        "nZq5\n"
        "-----END CERTIFICATE-----";

    const char testKey[] =
        "-----BEGIN PRIVATE KEY-----\n"
        "MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgw0VlkoyRhsiGGmYa\n"
        "R4bbSCCAbt0MrRex/LkuoYW1/myhRANCAARfsZNKls055081/xImV4diaUrCimYW\n"
        "2k0m7Rhq/B5xBjWP5ETfBy3XaoUXGA02bImZaTSTLd43TWTCB5IbRkng\n"
        "-----END PRIVATE KEY-----";
}

STD_TEST_SUITE(HttpFileServe) {
    STD_TEST(_ServeFiles) {
        auto exec = CoroExecutor::create(8);
        auto pool = ObjPool::fromMemory();
        auto sslCtx = SslCtx::create(pool.mutPtr(), StringView(testCert), StringView(testKey));

        struct Handler: HttpServe {
            CoroExecutor* exec;
            SslCtx* sslCtx;

            SslCtx* ssl() override {
                return sslCtx;
            }

            void serve(HttpServerResponse& resp) override {
                auto req = resp.request();

                ScopedFD fd(::open(Buffer(req->path()).cStr(), O_RDONLY));

                if (fd.get() < 0) {
                    resp.setStatus(404);
                    resp.addHeader(StringView("Content-Length"), StringView("0"));
                    resp.endHeaders();

                    return;
                }

                resp.endHeaders();

                CoroFDInput(fd, exec).sendTo(*resp.out());
                resp.out()->finish();
            }
        } handler;

        handler.exec = exec.mutPtr();
        handler.sslCtx = sslCtx;

        u16 port = 18080;

        if (auto* v = _ctx.args().find(StringView("port"))) {
            port = (u16)v->stou();
        }

        auto addr = makeAddr(port);

        WaitGroup wg(exec.mutPtr());

        serve(pool.mutPtr(), {
                                 .handler = &handler,
                                 .exec = exec.mutPtr(),
                                 .addr = (const sockaddr*)&addr,
                                 .addrLen = sizeof(addr),
                                 .wg = &wg,
                             });

        exec->spawn([&] {
            wg.wait();
        });

        exec->join();
    }

    STD_TEST(_ServeOK) {
        auto exec = CoroExecutor::create(8);
        auto pool = ObjPool::fromMemory();
        auto sslCtx = SslCtx::create(pool.mutPtr(), StringView(testCert), StringView(testKey));

        struct Handler: HttpServe {
            CoroExecutor* exec;
            SslCtx* sslCtx;

            SslCtx* ssl() override {
                return sslCtx;
            }

            void serve(HttpServerResponse& resp) override {
                auto req = resp.request();

                resp.setStatus(200);
                resp.addHeader(StringView("Content-Length"), StringView("0"));
                resp.endHeaders();
                resp.out()->finish();
            }
        } handler;

        handler.exec = exec.mutPtr();
        handler.sslCtx = sslCtx;

        u16 port = 18080;

        if (auto* v = _ctx.args().find(StringView("port"))) {
            port = (u16)v->stou();
        }

        auto addr = makeAddr(port);

        WaitGroup wg(exec.mutPtr());

        serve(pool.mutPtr(), {
                                 .handler = &handler,
                                 .exec = exec.mutPtr(),
                                 .addr = (const sockaddr*)&addr,
                                 .addrLen = sizeof(addr),
                                 .wg = &wg,
                             });

        exec->spawn([&] {
            wg.wait();
        });

        exec->join();
    }
}
