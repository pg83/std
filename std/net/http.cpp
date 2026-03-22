#include "http.h"

#include <std/thr/tcp.h>
#include <std/sys/crt.h>
#include <std/thr/coro.h>
#include <std/alg/defer.h>
#include <std/ios/input.h>
#include <std/dbg/insist.h>
#include <std/dbg/verify.h>
#include <std/ios/in_buf.h>
#include <std/ios/output.h>
#include <std/lib/buffer.h>
#include <std/mem/obj_pool.h>
#include <std/ios/stream_tcp.h>

#include <sys/socket.h>
#include <netinet/in.h>

using namespace stl;

namespace {
    struct HttpConnection {
        ObjPool::Ref pool;
        TcpSocket sock;
        TcpStream stream;
        InBuf buf;
        Buffer line;
        Buffer lcName;

        HttpConnection(CoroExecutor* exec, int fd);
        ~HttpConnection();

        bool serve(HttpServe& handler);
    };
}

HttpConnection::HttpConnection(CoroExecutor* exec, int fd)
    : pool(ObjPool::fromMemory())
    , sock(fd, exec)
    , stream(sock)
    , buf(stream)
{
}

HttpConnection::~HttpConnection() {
    sock.close();
}

bool HttpConnection::serve(HttpServe& handler) {
    pool = ObjPool::fromMemory();

    line.reset();

    if (!buf.readLine(line)) {
        return false;
    }

    HttpRequest req;

    req.in = &buf;
    req.out = &stream;

    StringView method, rest, path, version;

    STD_VERIFY(StringView(line).stripCr().split(' ', method, rest));

    rest.split(' ', path, version);

    req.method = pool->intern(method);
    req.path = pool->intern(path.empty() ? rest : path);

    for (;;) {
        line.reset();
        buf.readLine(line);

        StringView name, val;

        if (!StringView(line).stripCr().split(':', name, val)) {
            break;
        }

        lcName.grow(name.length());
        req.headers.insert(name.lower((u8*)lcName.mutData()), pool->intern(val.stripSpace()));
    }

    handler.serve(req);

    auto connection = req.headers.find(StringView("connection"));
    bool keepAlive = connection
        ? (*connection == StringView("keep-alive"))
        : (version == StringView("HTTP/1.1"));

    return keepAlive;
}

void stl::serve(HttpServe& handler, CoroExecutor* exec, const sockaddr* addr, u32 addrLen) {
    sockaddr_storage addrCopy{};
    memCpy(&addrCopy, addr, addrLen);

    TcpSocket srv(exec);

    STD_VERIFY(srv.socket(AF_INET, SOCK_STREAM, 0) == 0);

    srv.setReuseAddr(true);

    STD_VERIFY(srv.bind((const sockaddr*)&addrCopy, addrLen) == 0);
    STD_VERIFY(srv.listen(128) == 0);

    exec->spawnRun(SpawnParams().setSystem(true).setRunable([&handler, exec, srvFd = srv.fd] {
        TcpSocket srv(srvFd, exec);

        STD_DEFER {
            srv.close();
        };

        for (;;) {
            TcpSocket client;

            if (srv.acceptInf(client, nullptr, nullptr) != 0) {
                break;
            }

            exec->spawn([&handler, exec, fd = client.fd] {
                HttpConnection conn(exec, fd);

                while (conn.serve(handler)) {
                }
            });
        }
    }));
}
