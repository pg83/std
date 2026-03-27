#include "http_srv.h"

#include "http_io.h"
#include "tcp_socket.h"
#include "ssl_socket.h"
#include "http_reason.h"

#include <std/sys/fd.h>
#include <std/sys/crt.h>
#include <std/ios/sys.h>
#include <std/thr/coro.h>
#include <std/alg/defer.h>
#include <std/ios/input.h>
#include <std/sym/s_map.h>
#include <std/sys/throw.h>
#include <std/sys/atomic.h>
#include <std/dbg/insist.h>
#include <std/dbg/verify.h>
#include <std/ios/in_buf.h>
#include <std/ios/output.h>
#include <std/lib/buffer.h>
#include <std/lib/vector.h>
#include <std/sys/atomic.h>
#include <std/thr/poller.h>
#include <std/ios/out_buf.h>
#include <std/str/builder.h>
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
    struct HttpConnection;

    struct HttpRequestImpl: public HttpRequest {
        ObjPool::Ref pool = ObjPool::fromMemory();
        HttpConnection* conn;
        StringView reqMethod;
        StringView reqPath;
        StringView reqQuery;
        SymbolMap<StringView> headers;
        ZeroCopyInput* reqIn;
        bool keepAlive = false;

        HttpRequestImpl(HttpConnection* conn);

        bool serve();

        StringView path() override;
        StringView query() override;
        StringView method() override;
        ZeroCopyInput* in() override;
        StringView* header(StringView name) override;
    };

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
        HttpServe* handler;
        TcpSocket sock;
        ZeroCopyInput* in;
        Output* out;
        Buffer line;
        Buffer lcName;

        HttpConnection(HttpServe* handler, CoroExecutor* exec, int fd);
        ~HttpConnection();

        bool serve();
    };

    struct HttpResponseImpl: public HttpResponse {
        struct Header {
            StringView name;
            StringView value;
        };

        HttpRequestImpl* req;
        Output* rawOut;
        Vector<Header*> headers;
        SymbolMap<Header*> headerIndex;
        u32 status;

        HttpResponseImpl(HttpRequestImpl* req);

        void serialize(ZeroCopyOutput& out);

        Output* out() override;
        void endHeaders() override;
        HttpRequest* request() override;
        void setStatus(u32 code) override;
        void addHeader(StringView name, StringView value) override;
    };
}

HttpRequestImpl::HttpRequestImpl(HttpConnection* conn)
    : conn(conn)
{
    StringView method, rest, path, version;

    STD_VERIFY(StringView(conn->line).stripCr().split(' ', method, rest));

    rest.split(' ', path, version);

    reqMethod = pool->intern(method);

    StringView rawPath = path.empty() ? rest : path;
    StringView pathPart, queryPart;

    if (rawPath.split('?', pathPart, queryPart)) {
        reqPath = pool->intern(pathPart);
        reqQuery = pool->intern(queryPart);
    } else {
        reqPath = pool->intern(rawPath);
    }

    version = pool->intern(version);

    for (;;) {
        conn->line.reset();
        conn->in->readLine(conn->line);

        StringView name, val;

        if (!StringView(conn->line).stripCr().split(':', name, val)) {
            break;
        }

        headers.insert(name.lower(conn->lcName), pool->intern(val.stripSpace()));
    }

    if (auto* h = headers.find(StringView("connection")); h) {
        keepAlive = h->lower(conn->line) == StringView("keep-alive");
    } else {
        keepAlive = version.lower(conn->line) == StringView("http/1.1");
    }

    if (auto te = headers.find(StringView("transfer-encoding")); te && te->lower(conn->line) == StringView("chunked")) {
        reqIn = createChunkedInput(pool.mutPtr(), conn->in);
    } else if (auto cl = headers.find(StringView("content-length")); cl) {
        reqIn = createLimitedInput(pool.mutPtr(), conn->in, cl->stou());
    } else {
        reqIn = pool->make<ZeroInput>();
    }
}

bool HttpRequestImpl::serve() {
    HttpResponseImpl resp(this);

    conn->handler->serve(resp);
    reqIn->drain();

    return keepAlive;
}

StringView HttpRequestImpl::method() {
    return reqMethod;
}

StringView HttpRequestImpl::path() {
    return reqPath;
}

StringView HttpRequestImpl::query() {
    return reqQuery;
}

ZeroCopyInput* HttpRequestImpl::in() {
    return reqIn;
}

StringView* HttpRequestImpl::header(StringView name) {
    return headers.find(name);
}

HttpResponseImpl::HttpResponseImpl(HttpRequestImpl* req)
    : req(req)
    , rawOut(req->conn->out)
    , status(200)
{
}

Output* HttpResponseImpl::out() {
    return rawOut;
}

HttpRequest* HttpResponseImpl::request() {
    return req;
}

void HttpResponseImpl::setStatus(u32 code) {
    status = code;
}

void HttpResponseImpl::addHeader(StringView name, StringView value) {
    auto h = req->pool.mutPtr()->make<Header>();

    h->name = name;
    h->value = value;

    headers.pushBack(h);
    headerIndex.insert(name.lower(req->conn->lcName), h);
}

void HttpResponseImpl::serialize(ZeroCopyOutput& out) {
    out << StringView(u8"HTTP/1.1 ")
        << (u64)status
        << StringView(u8" ")
        << reasonPhrase(status)
        << StringView(u8"\r\n");

    for (auto* it = headers.begin(); it != headers.end(); ++it) {
        out << (*it)->name << StringView(u8": ") << (*it)->value << StringView(u8"\r\n");
    }

    out << StringView(u8"\r\n");
}

void HttpResponseImpl::endHeaders() {
    auto* pool = req->pool.mutPtr();

    if (req->keepAlive && !headerIndex.find(StringView("content-length")) && !headerIndex.find(StringView("transfer-encoding"))) {
        addHeader(StringView("Transfer-Encoding"), StringView("chunked"));
    }

    {
        StringBuilder sb;

        sb.xchg(req->conn->line);
        sb.reset();
        serialize(sb);
        rawOut->write(sb.data(), sb.used());
        sb.xchg(req->conn->line);
    }

    if (auto* cl = headerIndex.find(StringView("content-length")); cl) {
        rawOut = createLimitedOutput(pool, rawOut, (*cl)->value.stou());
    } else if (auto* te = headerIndex.find(StringView("transfer-encoding")); te && (*te)->value == StringView("chunked")) {
        rawOut = createChunkedOutput(pool, rawOut);
    }

    if (auto* conn = headerIndex.find(StringView("connection")); conn) {
        if ((*conn)->value != StringView("keep-alive")) {
            req->keepAlive = false;
        }
    }
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
        ScopedFD client;

        if (srv.acceptInf(client, nullptr, nullptr) != 0) {
            break;
        }

        if (stdAtomicFetch(&stopped, stl::MemoryOrder::Acquire)) {
            break;
        }

        exec->spawn([this, fd = client.release()] {
            try {
                TcpSocket(fd.get(), exec).setNoDelay(true);

                HttpConnection conn(&handler, exec, fd.get());

                while (conn.serve()) {
                    conn.out->flush();
                }

                conn.out->finish();
            } catch (...) {
                sysE << Exception::current() << endL << flsH;
            }
        });
    }
}

HttpConnection::HttpConnection(HttpServe* handler, CoroExecutor* exec, int fd)
    : handler(handler)
    , sock(fd, exec)
{
    auto stream = opool->make<TcpStream>(sock);

    Input* in = stream;
    out = opool->make<OutBuf>(*stream);

    if (auto ssl = handler->ssl()) {
        unsigned char b;

        exec->poll(fd, PollFlag::In);

        if (::recv(fd, &b, 1, MSG_PEEK) == 1 && b == 0x16) {
            auto s = ssl->create(opool.mutPtr(), in, out);

            in = s;
            out = s;
        }
    }

    this->in = opool->make<InBuf>(*in);
}

HttpConnection::~HttpConnection() {
    try {
        out->finish();
    } catch (...) {
        sysE << Exception::current() << endL << flsH;
    }

    sock.close();
}

bool HttpConnection::serve() {
    line.reset();

    if (!in->readLine(line)) {
        return false;
    }

    return HttpRequestImpl(this).serve();
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
