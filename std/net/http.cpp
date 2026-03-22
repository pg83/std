#include "http.h"
#include "io.h"

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
#include <std/sys/atomic.h>
#include <std/ios/in_zero.h>
#include <std/mem/obj_pool.h>
#include <std/thr/semaphore.h>
#include <std/ios/stream_tcp.h>
#include <std/thr/wait_group.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace stl;

namespace {
    struct HttpServerCtlImpl: public HttpServerCtl {
        HttpServe& handler;
        CoroExecutor* exec;
        sockaddr_storage addr;
        u32 addrLen;
        bool stopped;

        HttpServerCtlImpl(HttpServe& handler, CoroExecutor* exec, const sockaddr* addr, u32 addrLen);

        void stop() override;
        void run(Semaphore* sem);
    };

    struct HttpConnection {
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

HttpServerCtlImpl::HttpServerCtlImpl(HttpServe& handler, CoroExecutor* exec, const sockaddr* addr, u32 addrLen)
    : handler(handler)
    , exec(exec)
    , addr{}
    , addrLen(addrLen)
    , stopped(false)
{
    memCpy(&this->addr, addr, addrLen);
}

void HttpServerCtlImpl::stop() {
    stdAtomicStore(&stopped, true, stl::MemoryOrder::Release);

    exec->spawn([this] {
        TcpSocket sock(exec);

        if (sock.socket(addr.ss_family, SOCK_STREAM, 0) == 0) {
            sock.connectInf((const sockaddr*)&addr, addrLen);
            sock.close();
        }
    });
}

void HttpServerCtlImpl::run(Semaphore* sem) {
    TcpSocket srv(exec);

    STD_DEFER {
        srv.close();
    };

    STD_VERIFY(srv.socket(AF_INET, SOCK_STREAM, 0) == 0);

    srv.setReuseAddr(true);

    STD_VERIFY(srv.bind((const sockaddr*)&addr, addrLen) == 0);
    STD_VERIFY(srv.listen(128) == 0);

    sem->post();

    for (;;) {
        TcpSocket client;

        if (srv.acceptInf(client, nullptr, nullptr) != 0) {
            break;
        }

        if (stdAtomicFetch(&stopped, stl::MemoryOrder::Acquire)) {
            client.close();
            break;
        }

        exec->spawn([this, fd = client.fd] {
            HttpConnection conn(exec, fd);

            while (conn.serve(handler)) {
            }
        });
    }
}

HttpConnection::HttpConnection(CoroExecutor* exec, int fd)
    : sock(fd, exec)
    , stream(sock)
    , buf(stream)
{
}

HttpConnection::~HttpConnection() {
    sock.close();
}

bool HttpConnection::serve(HttpServe& handler) {
    auto pool = ObjPool::fromMemory();

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

    StringView rawPath = path.empty() ? rest : path;
    StringView pathPart, queryPart;

    if (rawPath.split('?', pathPart, queryPart)) {
        req.path = pool->intern(pathPart);
        req.query = pool->intern(queryPart);
    } else {
        req.path = pool->intern(rawPath);
    }

    version = pool->intern(version);

    for (;;) {
        line.reset();
        buf.readLine(line);

        StringView name, val;

        if (!StringView(line).stripCr().split(':', name, val)) {
            break;
        }

        req.headers.insert(name.lower(lcName), pool->intern(val.stripSpace()));
    }

    if (auto te = req.headers.find(StringView("transfer-encoding")); te && *te == StringView("chunked")) {
        req.in = createChunked(pool.mutPtr(), &buf);
    } else if (auto cl = req.headers.find(StringView("content-length")); cl) {
        req.in = createLimited(pool.mutPtr(), &buf, cl->stou());
    } else {
        req.in = pool->make<ZeroInput>();
    }

    handler.serve(req);

    {
        const void* chunk;
        size_t n;

        while ((n = req.in->next(&chunk))) {
            req.in->commit(n);
        }
    }

    bool keepAlive = false;

    if (auto connection = req.headers.find(StringView("connection")); connection) {
        keepAlive = connection->lower(line) == StringView("keep-alive");
    } else {
        keepAlive = version.lower(line) == StringView("http/1.1");
    }

    return keepAlive;
}

IntrusivePtr<HttpServerCtl> stl::serve(HttpServe& handler, CoroExecutor* exec, const sockaddr* addr, u32 addrLen, WaitGroup& wg) {
    auto ctl = makeIntrusivePtr(new HttpServerCtlImpl(handler, exec, addr, addrLen));

    wg.inc();

    Semaphore sem(0);

    exec->spawn([ctl, &wg, &sem] mutable {
        ctl->run(&sem);
        wg.done();
    });

    sem.wait();

    return ctl.mutPtr();
}

HttpServerCtl::~HttpServerCtl() {
}
