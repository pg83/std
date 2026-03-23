#include "socket.h"

#include <std/tst/ut.h>
#include <std/thr/coro.h>
#include <std/thr/async.h>
#include <std/alg/defer.h>
#include <std/dbg/insist.h>

#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

STD_TEST_SUITE(TcpSocket) {
    STD_TEST(ConnectAcceptEcho) {
        auto exec = CoroExecutor::create(4);

        TcpSocket srv(exec.mutPtr());
        STD_INSIST(srv.socket(AF_INET, SOCK_STREAM, 0) == 0);
        STD_DEFER {
            srv.close();
        };

        int opt = 1;
        ::setsockopt(srv.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        auto addr = makeAddr(17654);
        STD_INSIST(srv.bind((sockaddr*)&addr, sizeof(addr)) == 0);
        STD_INSIST(srv.listen(8) == 0);

        char recvBuf[32] = {};

        exec->spawn([&] {
            TcpSocket client;
            STD_INSIST(srv.acceptInf(client, nullptr, nullptr) == 0);
            STD_DEFER {
                client.close();
            };

            char buf[32] = {};
            size_t n = 0;
            STD_INSIST(client.readInf(&n, buf, sizeof(buf)) == 0);
            STD_INSIST(n > 0);
            size_t w = 0;
            STD_INSIST(client.writeInf(&w, buf, n) == 0);
            STD_INSIST(w == n);
        });

        exec->spawn([&] {
            TcpSocket cli(exec.mutPtr());
            auto caddr = makeAddr(17654);
            STD_INSIST(cli.connectInf((sockaddr*)&caddr, sizeof(caddr)) == 0);
            STD_DEFER {
                cli.close();
            };

            const char* msg = "hello";
            size_t msgLen = strlen(msg);
            size_t w = 0;
            STD_INSIST(cli.writeInf(&w, msg, msgLen) == 0);
            STD_INSIST(w == msgLen);

            size_t n = 0;
            STD_INSIST(cli.readInf(&n, recvBuf, sizeof(recvBuf)) == 0);
            STD_INSIST(n == msgLen);
        });

        exec->join();

        STD_INSIST(memcmp(recvBuf, "hello", 5) == 0);
    }

    STD_TEST(AcceptTimeout) {
        auto exec = CoroExecutor::create(4);

        TcpSocket srv(exec.mutPtr());
        STD_INSIST(srv.socket(AF_INET, SOCK_STREAM, 0) == 0);
        STD_DEFER {
            srv.close();
        };

        int opt = 1;
        ::setsockopt(srv.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        auto addr = makeAddr(17655);
        STD_INSIST(srv.bind((sockaddr*)&addr, sizeof(addr)) == 0);
        STD_INSIST(srv.listen(8) == 0);

        auto f = async(exec.mutPtr(), [&] {
            TcpSocket client;
            return srv.acceptTout(client, nullptr, nullptr, 1);
        });

        STD_INSIST(f.wait() != 0);

        exec->join();
    }
}
