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

#include <sys/socket.h>
#include <netinet/in.h>

using namespace stl;

HttpRequest::HttpRequest(Input& in, Output& out) noexcept
    : in(in)
    , out(out)
{
}

namespace {
    void parseRequest(InBuf& buf, HttpRequest& req, ObjPool& pool) {
        Buffer line;

        STD_INSIST(buf.readLine(line));

        StringView method, rest, path, version;

        STD_INSIST(StringView(line).stripCr().split(' ', method, rest));

        rest.split(' ', path, version);
        req.method = pool.intern(method);
        req.path = pool.intern(path.empty() ? rest : path);

        for (;;) {
            line.reset();
            buf.readLine(line);

            StringView name, val;

            if (!StringView(line).stripCr().split(':', name, val)) {
                break;
            }

            req.headers.insert(pool.intern(name), pool.intern(val.stripSpace()));
        }
    }

    void serveConn(HttpServe& handler, CoroExecutor* exec, int fd) {
        TcpSocket sock(fd, exec);

        STD_DEFER {
            sock.close();
        };

        TcpStream stream(sock);
        InBuf buf(stream);
        ObjPool::Ref pool = ObjPool::fromMemory();
        HttpRequest req(buf, stream);

        parseRequest(buf, req, *pool);
        handler.serve(req);
    }
}

void stl::serve(HttpServe& handler, CoroExecutor* exec, const sockaddr* addr, u32 addrLen) {
    sockaddr_storage addrCopy{};
    memCpy(&addrCopy, addr, addrLen);

    TcpSocket srv(exec);

    STD_INSIST(srv.socket(AF_INET, SOCK_STREAM, 0) == 0);

    int opt = 1;
    ::setsockopt(srv.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    STD_INSIST(srv.bind((const sockaddr*)&addrCopy, addrLen) == 0);
    STD_INSIST(srv.listen(128) == 0);

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
