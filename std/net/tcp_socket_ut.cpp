#include "tcp_socket.h"

#include <std/sys/fd.h>
#include <std/tst/ut.h>
#include <std/thr/coro.h>
#include <std/thr/async.h>
#include <std/dbg/insist.h>
#include <std/mem/obj_pool.h>
#include <std/thr/coro_config.h>

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

STD_TEST_SUITE(TcpSocket) {
    STD_TEST(ConnectAcceptEcho) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

        auto srv = TcpSocket::create(pool.mutPtr(), exec);
        STD_INSIST(srv->socket(AF_INET, SOCK_STREAM, 0) == 0);
        srv->setReuseAddr(true);

        auto addr = makeAddr(17654);
        STD_INSIST(srv->bind((sockaddr*)&addr, sizeof(addr)) == 0);
        STD_INSIST(srv->listen(8) == 0);

        char recvBuf[32] = {};

        exec->spawn([&] {
            ScopedFD clientFd;
            STD_INSIST(srv->acceptInf(clientFd, nullptr, nullptr) == 0);

            TcpSocket client(clientFd.get(), exec);

            char buf[32] = {};
            size_t n = 0;
            STD_INSIST(client.readInf(&n, buf, sizeof(buf)) == 0);
            STD_INSIST(n > 0);
            size_t w = 0;
            STD_INSIST(client.writeInf(&w, buf, n) == 0);
            STD_INSIST(w == n);
        });

        exec->spawn([&] {
            auto cpool = ObjPool::fromMemory();
            auto cli = TcpSocket::create(cpool.mutPtr(), exec);
            auto caddr = makeAddr(17654);
            STD_INSIST(cli->connectInf((sockaddr*)&caddr, sizeof(caddr)) == 0);

            const char* msg = "hello";
            size_t msgLen = strlen(msg);
            size_t w = 0;
            STD_INSIST(cli->writeInf(&w, msg, msgLen) == 0);
            STD_INSIST(w == msgLen);

            size_t n = 0;
            STD_INSIST(cli->readInf(&n, recvBuf, sizeof(recvBuf)) == 0);
            STD_INSIST(n == msgLen);
        });

        exec->join();

        STD_INSIST(memcmp(recvBuf, "hello", 5) == 0);
    }

    STD_TEST(AcceptTimeout) {
        auto pool = ObjPool::fromMemory();
        auto exec = CoroExecutor::create(pool.mutPtr(), 4);

        auto srv = TcpSocket::create(pool.mutPtr(), exec);
        STD_INSIST(srv->socket(AF_INET, SOCK_STREAM, 0) == 0);
        srv->setReuseAddr(true);

        auto addr = makeAddr(17655);
        STD_INSIST(srv->bind((sockaddr*)&addr, sizeof(addr)) == 0);
        STD_INSIST(srv->listen(8) == 0);

        auto f = async(exec, [&] {
            ScopedFD client;
            return srv->acceptTout(client, nullptr, nullptr, 1);
        });

        STD_INSIST(f.wait() != 0);

        exec->join();
    }
}
