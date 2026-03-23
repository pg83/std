#include "http.h"

#include <std/sys/fd.h>
#include <std/tst/ut.h>
#include <std/thr/tcp.h>
#include <std/alg/defer.h>
#include <std/dbg/insist.h>
#include <std/ios/out_buf.h>
#include <std/thr/wait_group.h>
#include <std/ios/stream_tcp.h>

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

STD_TEST_SUITE(HttpRequestParsing) {
    STD_TEST(PathNoQuery) {
        auto exec = CoroExecutor::create(4);

        struct Handler: HttpServe {
            StringView path;
            StringView query;

            void serve(HttpRequest& req) override {
                path = req.path;
                query = req.query;
                const char* resp = "HTTP/1.0 200 OK\r\nContent-Length: 0\r\n\r\n";
                req.out->write(resp, ::strlen(resp));
            }
        } handler;

        auto addr = makeAddr(17670);
        WaitGroup wg(exec.mutPtr());
        auto ctl = serve(handler, exec.mutPtr(), (const sockaddr*)&addr, sizeof(addr), wg);

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

        STD_INSIST(handler.path == StringView("/foo/bar"));
        STD_INSIST(handler.query.empty());
    }

    STD_TEST(PathWithQuery) {
        auto exec = CoroExecutor::create(4);

        struct Handler: HttpServe {
            StringView path;
            StringView query;

            void serve(HttpRequest& req) override {
                path = req.path;
                query = req.query;
                const char* resp = "HTTP/1.0 200 OK\r\nContent-Length: 0\r\n\r\n";
                req.out->write(resp, ::strlen(resp));
            }
        } handler;

        auto addr = makeAddr(17671);
        WaitGroup wg(exec.mutPtr());
        auto ctl = serve(handler, exec.mutPtr(), (const sockaddr*)&addr, sizeof(addr), wg);

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

        STD_INSIST(handler.path == StringView("/search"));
        STD_INSIST(handler.query == StringView("q=hello&page=2"));
    }
}

STD_TEST_SUITE(HttpServer) {
    STD_TEST(GetRequest) {
        auto exec = CoroExecutor::create(4);

        struct Handler: HttpServe {
            void serve(HttpRequest& req) override {
                const char* resp =
                    "HTTP/1.0 200 OK\r\n"
                    "Content-Length: 5\r\n"
                    "\r\n"
                    "hello";
                req.out->write(resp, ::strlen(resp));
            }
        } handler;

        auto addr = makeAddr(17661);
        WaitGroup wg(exec.mutPtr());
        auto ctl = serve(handler, exec.mutPtr(), (const sockaddr*)&addr, sizeof(addr), wg);

        char recvBuf[256] = {};

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

            size_t n = 0;
            cli.readInf(&n, recvBuf, sizeof(recvBuf) - 1);
            STD_INSIST(n > 0);

            ctl->stop();
        });

        exec->spawn([&] {
            wg.wait();
        });

        exec->join();

        STD_INSIST(StringView(recvBuf).search(StringView(u8"hello")) != nullptr);
    }

    STD_TEST(KeepAliveChunked) {
        auto exec = CoroExecutor::create(4);

        struct Handler: HttpServe {
            void serve(HttpRequest& req) override {
                const char* resp =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Length: 2\r\n"
                    "\r\n"
                    "ok";
                req.out->write(resp, ::strlen(resp));
            }
        } handler;

        auto addr = makeAddr(17663);
        WaitGroup wg(exec.mutPtr());
        auto ctl = serve(handler, exec.mutPtr(), (const sockaddr*)&addr, sizeof(addr), wg);

        char buf1[256] = {};
        char buf2[256] = {};

        exec->spawn([&] {
            TcpSocket cli(exec.mutPtr());
            auto caddr = makeAddr(17663);
            STD_INSIST(cli.connectInf((const sockaddr*)&caddr, sizeof(caddr)) == 0);
            STD_DEFER {
                cli.close();
            };

            TcpStream stream(cli);
            const char* req1 =
                "POST /a HTTP/1.1\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n"
                "5\r\n"
                "hello\r\n"
                "0\r\n"
                "\r\n";
            stream.write(req1, ::strlen(req1));

            size_t n1 = 0;
            cli.readInf(&n1, buf1, sizeof(buf1) - 1);
            STD_INSIST(n1 > 0);

            const char* req2 =
                "POST /b HTTP/1.1\r\n"
                "Transfer-Encoding: chunked\r\n"
                "\r\n"
                "5\r\n"
                "world\r\n"
                "0\r\n"
                "\r\n";
            stream.write(req2, ::strlen(req2));

            size_t n2 = 0;
            cli.readInf(&n2, buf2, sizeof(buf2) - 1);
            STD_INSIST(n2 > 0);

            ctl->stop();
        });

        exec->spawn([&] {
            wg.wait();
        });

        exec->join();

        STD_INSIST(StringView(buf1).search(StringView(u8"ok")) != nullptr);
        STD_INSIST(StringView(buf2).search(StringView(u8"ok")) != nullptr);
    }

    STD_TEST(KeepAlive) {
        auto exec = CoroExecutor::create(4);

        struct Handler: HttpServe {
            void serve(HttpRequest& req) override {
                const char* resp =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Length: 2\r\n"
                    "\r\n"
                    "ok";
                req.out->write(resp, ::strlen(resp));
            }
        } handler;

        auto addr = makeAddr(17662);
        WaitGroup wg(exec.mutPtr());
        auto ctl = serve(handler, exec.mutPtr(), (const sockaddr*)&addr, sizeof(addr), wg);

        char buf1[256] = {};
        char buf2[256] = {};

        exec->spawn([&] {
            TcpSocket cli(exec.mutPtr());
            auto caddr = makeAddr(17662);
            STD_INSIST(cli.connectInf((const sockaddr*)&caddr, sizeof(caddr)) == 0);
            STD_DEFER {
                cli.close();
            };

            TcpStream stream(cli);
            const char* req1 =
                "GET /a HTTP/1.1\r\n"
                "Content-Length: 0\r\n"
                "\r\n";
            stream.write(req1, ::strlen(req1));

            size_t n1 = 0;
            cli.readInf(&n1, buf1, sizeof(buf1) - 1);
            STD_INSIST(n1 > 0);

            const char* req2 =
                "GET /b HTTP/1.1\r\n"
                "Content-Length: 0\r\n"
                "\r\n";
            stream.write(req2, ::strlen(req2));

            size_t n2 = 0;
            cli.readInf(&n2, buf2, sizeof(buf2) - 1);
            STD_INSIST(n2 > 0);

            ctl->stop();
        });

        exec->spawn([&] {
            wg.wait();
        });

        exec->join();

        STD_INSIST(StringView(buf1).search(StringView(u8"ok")) != nullptr);
        STD_INSIST(StringView(buf2).search(StringView(u8"ok")) != nullptr);
    }
}

STD_TEST_SUITE(HttpFileServe) {
    STD_TEST(_ServeFiles) {
        auto exec = CoroExecutor::create(8);

        struct Handler: HttpServe {
            CoroExecutor* exec;

            void serve(HttpRequest& req) override {
                ScopedFD fd(::open(Buffer(req.path).cStr(), O_RDONLY));
                if (fd.get() < 0) {
                    const char* r = "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                    req.out->write(r, ::strlen(r));
                    return;
                }

                Buffer buf;
                for (;;) {
                    buf.growDelta(64 * 1024);
                    ssize_t n = exec->pread(fd.get(), buf.mutCurrent(), buf.left(), buf.used());
                    if (n <= 0) {
                        break;
                    }
                    buf.seekRelative(n);
                }

                OutBuf out(*req.out);
                out << StringView("HTTP/1.0 200 OK\r\nContent-Length: ")
                    << buf.used()
                    << StringView("\r\n\r\n");
                out.write(buf.data(), buf.used());
            }
        } handler;
        handler.exec = exec.mutPtr();

        auto addr = makeAddr(18080);
        WaitGroup wg(exec.mutPtr());
        serve(handler, exec.mutPtr(), (const sockaddr*)&addr, sizeof(addr), wg);

        exec->spawn([&] {
            wg.wait();
        });

        exec->join();
    }
}
