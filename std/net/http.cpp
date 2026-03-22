#include "http.h"

#include <std/ios/in_buf.h>
#include <std/ios/input.h>
#include <std/ios/output.h>
#include <std/ios/stream_tcp.h>
#include <std/lib/buffer.h>
#include <std/mem/obj_pool.h>
#include <std/sys/crt.h>
#include <std/thr/coro.h>
#include <std/thr/tcp.h>
#include <std/alg/defer.h>
#include <std/dbg/insist.h>
#include <std/dbg/verify.h>

#include <sys/socket.h>
#include <netinet/in.h>

using namespace stl;

namespace {
    struct HttpRequestImpl: public HttpRequest {
        HttpRequestImpl(ZeroCopyInput& in, Output& out, ObjPool& pool);
    };
}

HttpRequestImpl::HttpRequestImpl(ZeroCopyInput& in_, Output& out_, ObjPool& pool)
    : HttpRequest{.in = &in_, .out = &out_}
{
    Buffer line;

    STD_VERIFY(in->readLine(line));

    StringView rest;
    StringView path;
    StringView method;
    StringView version;

    STD_VERIFY(StringView(line).stripCr().split(' ', method, rest));

    rest.split(' ', path, version);

    this->method = pool.intern(method);
    this->path = pool.intern(path.empty() ? rest : path);

    Buffer lcName;

    for (;;) {
        line.reset();
        in->readLine(line);

        StringView name, val;

        if (!StringView(line).stripCr().split(':', name, val)) {
            break;
        }

        lcName.grow(name.length());
        headers.insert(name.lower((u8*)lcName.mutData()), pool.intern(val.stripSpace()));
    }
}

namespace {
    void serveConn(HttpServe& handler, CoroExecutor* exec, int fd) {
        TcpSocket sock(fd, exec);

        STD_DEFER {
            sock.close();
        };

        ObjPool::Ref pool = ObjPool::fromMemory();
        TcpStream stream(sock);
        InBuf buf(stream);
        HttpRequestImpl req(buf, stream, *pool);

        handler.serve(req);
    }
}

void stl::serve(HttpServe& handler, CoroExecutor* exec, const sockaddr* addr, u32 addrLen) {
    sockaddr_storage addrCopy{};
    memCpy(&addrCopy, addr, addrLen);

    TcpSocket srv(exec);

    STD_VERIFY(srv.socket(AF_INET, SOCK_STREAM, 0) == 0);

    int opt = 1;
    ::setsockopt(srv.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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
                serveConn(handler, exec, fd);
            });
        }
    }));
}
