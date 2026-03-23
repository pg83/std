#include "http.h"
#include "io.h"
#include "ssl.h"
#include "socket.h"

#include <std/sys/crt.h>
#include <std/thr/coro.h>
#include <std/alg/defer.h>
#include <std/ios/input.h>
#include <std/dbg/insist.h>
#include <std/dbg/verify.h>
#include <std/ios/in_buf.h>
#include <std/ios/output.h>
#include <std/ios/out_buf.h>
#include <std/lib/buffer.h>
#include <std/str/builder.h>
#include <std/sys/atomic.h>
#include <std/thr/poller.h>
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
    StringView reasonPhrase(u32 code) {
        switch (code) {
            case 200:
                return StringView(u8"OK");
            case 201:
                return StringView(u8"Created");
            case 204:
                return StringView(u8"No Content");
            case 301:
                return StringView(u8"Moved Permanently");
            case 302:
                return StringView(u8"Found");
            case 304:
                return StringView(u8"Not Modified");
            case 400:
                return StringView(u8"Bad Request");
            case 401:
                return StringView(u8"Unauthorized");
            case 403:
                return StringView(u8"Forbidden");
            case 404:
                return StringView(u8"Not Found");
            case 405:
                return StringView(u8"Method Not Allowed");
            case 500:
                return StringView(u8"Internal Server Error");
            case 502:
                return StringView(u8"Bad Gateway");
            case 503:
                return StringView(u8"Service Unavailable");
        }

        return StringView(u8"Unknown");
    }
}

struct stl::HttpResponseImpl {
    HttpRequest* req;
    Output* rawOut;
    Output* out;
    Buffer hdrs;
    SymbolMap<StringView> respHeaders;
    Buffer lcName;
    u32 status;

    HttpResponseImpl(HttpRequest* req);

    void setStatus(u32 code);
    void addHeader(StringView name, StringView value);
    void endHeaders();
};

HttpResponseImpl::HttpResponseImpl(HttpRequest* req)
    : req(req)
    , rawOut(req->out)
    , out(nullptr)
    , status(200)
{
}

void HttpResponseImpl::setStatus(u32 code) {
    status = code;
}

void HttpResponseImpl::addHeader(StringView name, StringView value) {
    hdrs.append(name.data(), name.length());
    hdrs.append(u8": ", 2);
    hdrs.append(value.data(), value.length());
    hdrs.append(u8"\r\n", 2);

    respHeaders.insert(name.lower(lcName), req->opool->intern(value));
}

void HttpResponseImpl::endHeaders() {
    auto* pool = req->opool;

    {
        StringBuilder sb;

        sb << StringView(u8"HTTP/1.1 ")
           << (u64)status
           << StringView(u8" ")
           << reasonPhrase(status)
           << StringView(u8"\r\n");

        sb.write(hdrs.data(), hdrs.used());
        sb << StringView(u8"\r\n");

        rawOut->write(sb.data(), sb.used());
    }

    out = rawOut;

    if (auto* cl = respHeaders.find(StringView("content-length")); cl) {
        out = createLimitedOutput(pool, rawOut, cl->stou());
    } else if (auto* te = respHeaders.find(StringView("transfer-encoding")); te && *te == StringView("chunked")) {
        out = createChunkedOutput(pool, rawOut);
    }

    // determine keep-alive
    bool ka = false;

    if (auto* conn = respHeaders.find(StringView("connection")); conn) {
        ka = *conn == StringView("keep-alive");
    } else if (auto* reqConn = req->headers.find(StringView("connection")); reqConn) {
        Buffer tmp;
        ka = reqConn->lower(tmp) == StringView("keep-alive");
    }

    req->keepAlive = ka;
}

HttpResponse::HttpResponse(HttpRequest& req)
    : impl(req.opool->make<HttpResponseImpl>(&req))
{
}

Output* HttpResponse::out() {
    return impl->out;
}

void HttpResponse::setStatus(u32 code) {
    impl->setStatus(code);
}

void HttpResponse::addHeader(StringView name, StringView value) {
    impl->addHeader(name, value);
}

void HttpResponse::endHeaders() {
    impl->endHeaders();
}

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
        ObjPool::Ref opool = ObjPool::fromMemory();
        TcpSocket sock;
        ZeroCopyInput* in;
        Output* out;
        Buffer line;
        Buffer lcName;

        HttpConnection(CoroExecutor* exec, int fd, SslCtx* ssl);
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
            try {
                HttpConnection conn(exec, fd, handler.ssl());

                while (conn.serve(handler)) {
                }
            } catch (...) {
            }
        });
    }
}

HttpConnection::HttpConnection(CoroExecutor* exec, int fd, SslCtx* ssl)
    : sock(fd, exec)
{
    auto* stream = opool->make<TcpStream>(sock);

    Input* rawIn = stream;
    Output* rawOut = stream;

    if (ssl) {
        unsigned char b;

        exec->poll(fd, PollFlag::In);

        if (::recv(fd, &b, 1, MSG_PEEK) == 1 && b == 0x16) {
            auto* s = ssl->create(opool.mutPtr(), rawIn, rawOut);

            rawIn = s;
            rawOut = s;
        }
    }

    in = opool->make<InBuf>(*rawIn);
    out = rawOut;
}

HttpConnection::~HttpConnection() {
    sock.close();
}

bool HttpConnection::serve(HttpServe& handler) {
    auto pool = ObjPool::fromMemory();

    line.reset();

    if (!in->readLine(line)) {
        return false;
    }

    HttpRequest req;

    req.opool = pool.mutPtr();
    req.in = in;
    req.out = out;
    req.keepAlive = false;

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
        in->readLine(line);

        StringView name, val;

        if (!StringView(line).stripCr().split(':', name, val)) {
            break;
        }

        req.headers.insert(name.lower(lcName), pool->intern(val.stripSpace()));
    }

    if (auto te = req.headers.find(StringView("transfer-encoding")); te && *te == StringView("chunked")) {
        req.in = createChunked(pool.mutPtr(), in);
    } else if (auto cl = req.headers.find(StringView("content-length")); cl) {
        req.in = createLimited(pool.mutPtr(), in, cl->stou());
    } else {
        req.in = pool->make<ZeroInput>();
    }

    handler.serve(req);

    req.in->drain();

    return req.keepAlive;
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

SslCtx* HttpServe::ssl() {
    return nullptr;
}
