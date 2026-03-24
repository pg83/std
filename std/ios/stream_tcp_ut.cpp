#include "stream_tcp.h"

#include <std/sys/fd.h>
#include <std/tst/ut.h>
#include <std/thr/coro.h>
#include <std/thr/async.h>
#include <std/alg/defer.h>
#include <std/net/socket.h>
#include <std/dbg/insist.h>

#include <string.h>
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

STD_TEST_SUITE(TcpStream) {
    STD_TEST(Echo) {
        auto exec = CoroExecutor::create(4);

        TcpSocket srv(exec.mutPtr());
        STD_INSIST(srv.socket(AF_INET, SOCK_STREAM, 0) == 0);
        STD_DEFER {
            srv.close();
        };

        int opt = 1;
        ::setsockopt(srv.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        auto addr = makeAddr(17660);
        STD_INSIST(srv.bind((sockaddr*)&addr, sizeof(addr)) == 0);
        STD_INSIST(srv.listen(8) == 0);

        char recvBuf[32] = {};

        exec->spawn([&] {
            ScopedFD clientFd;
            STD_INSIST(srv.acceptInf(clientFd, nullptr, nullptr) == 0);

            TcpSocket client(clientFd.get(), exec.mutPtr());
            TcpStream stream(client);

            char buf[32] = {};
            size_t n = stream.read(buf, sizeof(buf));
            STD_INSIST(n > 0);
            stream.write(buf, n);
        });

        exec->spawn([&] {
            TcpSocket cli(exec.mutPtr());
            auto caddr = makeAddr(17660);
            STD_INSIST(cli.connectInf((sockaddr*)&caddr, sizeof(caddr)) == 0);
            STD_DEFER {
                cli.close();
            };

            TcpStream stream(cli);

            const char* msg = "hello";
            size_t msgLen = strlen(msg);
            stream.write(msg, msgLen);

            size_t n = stream.read(recvBuf, sizeof(recvBuf));
            STD_INSIST(n == msgLen);
        });

        exec->join();

        STD_INSIST(memcmp(recvBuf, "hello", 5) == 0);
    }
}
