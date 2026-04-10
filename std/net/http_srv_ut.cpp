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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

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
        WaitGroup wg(exec);
        auto ctl = serve(pool.mutPtr(), {
                                            .handler = &handler,
                                            .exec = exec,
                                            .wg = &wg,
                                            .addr = (const sockaddr*)&addr,
                                            .addrLen = sizeof(addr),
                                        });

        exec->spawn([&] {
            auto caddr = makeAddr(17670);
            int cfd;
            STD_INSIST(TcpSocket::connectInf(&cfd, exec, (const sockaddr*)&caddr, sizeof(caddr)) == 0);
            TcpSocket cli(cfd, exec);
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

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
        WaitGroup wg(exec);
        auto ctl = serve(pool.mutPtr(), {
                                            .handler = &handler,
                                            .exec = exec,
                                            .wg = &wg,
                                            .addr = (const sockaddr*)&addr,
                                            .addrLen = sizeof(addr),
                                        });

        exec->spawn([&] {
            auto caddr = makeAddr(17671);
            int cfd;
            STD_INSIST(TcpSocket::connectInf(&cfd, exec, (const sockaddr*)&caddr, sizeof(caddr)) == 0);
            TcpSocket cli(cfd, exec);
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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

        struct Handler: HttpServe {
            void serve(HttpServerResponse& resp) override {
                resp.addHeader(StringView("Content-Length"), StringView("5"));
                resp.endHeaders();
                resp.out()->write("hello", 5);
            }
        } handler;

        auto addr = makeAddr(17661);
        WaitGroup wg(exec);
        auto ctl = serve(pool.mutPtr(), {
                                            .handler = &handler,
                                            .exec = exec,
                                            .wg = &wg,
                                            .addr = (const sockaddr*)&addr,
                                            .addrLen = sizeof(addr),
                                        });

        u32 respStatus = 0;
        Buffer respBody;

        exec->spawn([&] {
            auto caddr = makeAddr(17661);
            int cfd;
            STD_INSIST(TcpSocket::connectInf(&cfd, exec, (const sockaddr*)&caddr, sizeof(caddr)) == 0);
            TcpSocket cli(cfd, exec);
            STD_DEFER {
                cli.close();
            };

            TcpStream stream(cli);
            InBuf in(stream);
            auto pool = ObjPool::fromMemory();

            auto req = HttpClientRequest::create(pool.mutPtr(), &in, &stream);

            req->setPath(StringView("/index.html"));
            req->addHeader(StringView("Host"), StringView("localhost"));
            req->endHeaders();

            auto resp = req->response();

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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

        struct Handler: HttpServe {
            void serve(HttpServerResponse& resp) override {
                resp.addHeader(StringView("Content-Length"), StringView("2"));
                resp.addHeader(StringView("Connection"), StringView("keep-alive"));
                resp.endHeaders();
                resp.out()->write("ok", 2);
            }
        } handler;

        auto addr = makeAddr(17663);
        WaitGroup wg(exec);
        auto ctl = serve(pool.mutPtr(), {
                                            .handler = &handler,
                                            .exec = exec,
                                            .wg = &wg,
                                            .addr = (const sockaddr*)&addr,
                                            .addrLen = sizeof(addr),
                                        });

        Buffer body1;
        Buffer body2;

        exec->spawn([&] {
            auto caddr = makeAddr(17663);
            int cfd;
            STD_INSIST(TcpSocket::connectInf(&cfd, exec, (const sockaddr*)&caddr, sizeof(caddr)) == 0);
            TcpSocket cli(cfd, exec);
            STD_DEFER {
                cli.close();
            };

            TcpStream stream(cli);
            InBuf in(stream);
            auto pool = ObjPool::fromMemory();

            {
                auto req1 = HttpClientRequest::create(pool.mutPtr(), &in, &stream);

                req1->setMethod(StringView("POST"));
                req1->setPath(StringView("/a"));
                req1->endHeaders();
                req1->out()->write("hello", 5);

                auto resp1 = req1->response();

                STD_INSIST(resp1->status() == 200);
                resp1->body()->readAll(body1);
            }

            {
                auto req2 = HttpClientRequest::create(pool.mutPtr(), &in, &stream);

                req2->setMethod(StringView("POST"));
                req2->setPath(StringView("/b"));
                req2->endHeaders();
                req2->out()->write("world", 5);

                auto resp2 = req2->response();

                STD_INSIST(resp2->status() == 200);
                resp2->body()->readAll(body2);
            }

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
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

        struct Handler: HttpServe {
            void serve(HttpServerResponse& resp) override {
                resp.addHeader(StringView("Content-Length"), StringView("2"));
                resp.addHeader(StringView("Connection"), StringView("keep-alive"));
                resp.endHeaders();
                resp.out()->write("ok", 2);
            }
        } handler;

        auto addr = makeAddr(17662);
        WaitGroup wg(exec);
        auto ctl = serve(pool.mutPtr(), {
                                            .handler = &handler,
                                            .exec = exec,
                                            .wg = &wg,
                                            .addr = (const sockaddr*)&addr,
                                            .addrLen = sizeof(addr),
                                        });

        Buffer body1;
        Buffer body2;

        exec->spawn([&] {
            auto caddr = makeAddr(17662);
            int cfd;
            STD_INSIST(TcpSocket::connectInf(&cfd, exec, (const sockaddr*)&caddr, sizeof(caddr)) == 0);
            TcpSocket cli(cfd, exec);
            STD_DEFER {
                cli.close();
            };

            TcpStream stream(cli);
            InBuf in(stream);
            auto pool = ObjPool::fromMemory();

            {
                auto req1 = HttpClientRequest::create(pool.mutPtr(), &in, &stream);

                req1->setPath(StringView("/a"));
                req1->addHeader(StringView("Content-Length"), StringView("0"));
                req1->endHeaders();

                auto resp1 = req1->response();

                STD_INSIST(resp1->status() == 200);
                resp1->body()->readAll(body1);
            }

            {
                auto req2 = HttpClientRequest::create(pool.mutPtr(), &in, &stream);

                req2->setPath(StringView("/b"));
                req2->addHeader(StringView("Content-Length"), StringView("0"));
                req2->endHeaders();

                auto resp2 = req2->response();

                STD_INSIST(resp2->status() == 200);
                resp2->body()->readAll(body2);
            }

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
