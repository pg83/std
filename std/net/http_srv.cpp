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
#include <std/ios/in.h>
#include <std/ios/in_zc.h>
#include <std/ios/out.h>
#include <std/ios/output.h>
#include <std/lib/buffer.h>
#include <std/lib/vector.h>
#include <std/sys/atomic.h>
#include <std/sys/num_cpu.h>
#include <std/str/builder.h>
#include <std/mem/obj_pool.h>
#include <std/ios/stream_tcp.h>
#include <std/thr/wait_group.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace stl;

namespace {
    struct HttpConnection;

    struct HttpServerRequestImpl: public HttpServerRequest {
        ObjPool* pool;
        HttpConnection* conn;
        StringView reqMethod;
        StringView reqPath;
        StringView reqQuery;
        SymbolMap<StringView> headers{pool};
        ZeroCopyInput* reqIn;
        bool keepAlive = false;

        HttpServerRequestImpl(ObjPool* pool, HttpConnection* conn, StringView reqLine);

        bool serve();

        StringView path() override;
        StringView query() override;
        StringView method() override;
        ZeroCopyInput* in() override;
        StringView* header(StringView name) override;
    };

    struct HttpServerCtlImpl: public HttpServerCtl {
        HttpServeOpts* opts_;
        bool stopped;

        HttpServerCtlImpl(HttpServeOpts* opts);

        void stop() override;
        TcpSocket* listen(ObjPool* pool);
        void run(TcpSocket* srv);
    };

    struct HttpConnection {
        HttpServeOpts* opts;
        ObjPool* pool;
        TcpSocket sock;
        ZeroCopyInput* in;
        Output* out;
        Buffer line;
        Buffer lcName;

        HttpConnection(HttpServeOpts* opts, ObjPool* pool, FD* client);

        void run();
        bool serve();
    };

    struct HttpServerResponseImpl: public HttpServerResponse {
        struct Header {
            StringView name;
            StringView value;
        };

        HttpServerRequestImpl* req;
        Output* rawOut;
        Vector<Header*> headers;
        SymbolMap<Header*> headerIndex{req->pool};
        u32 status;

        HttpServerResponseImpl(HttpServerRequestImpl* req);

        Output* out() override;
        void endHeaders() override;
        void setStatus(u32 code) override;
        void serialize(ZeroCopyOutput& out);
        HttpServerRequest* request() override;
        void addHeader(StringView name, StringView value) override;
    };

    HttpServeOpts* internOpts(ObjPool* pool, HttpServeOpts opts) {
        if (!opts.exec) {
            opts.exec = CoroExecutor::create(pool, numCpu());
        }

        if (!opts.wg) {
            opts.wg = WaitGroup::create(pool, 0);
        }

        auto storage = pool->make<sockaddr_storage>();

        memCpy(storage, opts.addr, opts.addrLen);

        opts.addr = (const sockaddr*)storage;

        return pool->make<HttpServeOpts>(opts);
    }
}

HttpServerRequestImpl::HttpServerRequestImpl(ObjPool* pool, HttpConnection* conn, StringView reqLine)
    : pool(pool)
    , conn(conn)
{
    StringView method, rest, path, version;

    STD_VERIFY(reqLine.stripCr().split(' ', method, rest));

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
        StringView hdrLine;

        if (!conn->in->readLineZc(hdrLine, conn->line)) {
            break;
        }

        StringView name, val;

        if (!hdrLine.stripCr().split(':', name, val)) {
            break;
        }

        headers.insert(name.lower(conn->lcName), pool->intern(val.stripSpace()));
    }

    if (auto h = headers.find(StringView("connection")); h) {
        keepAlive = h->lower(conn->line) == StringView("keep-alive");
    } else {
        keepAlive = version.lower(conn->line) == StringView("http/1.1");
    }

    if (auto te = headers.find(StringView("transfer-encoding")); te && te->lower(conn->line) == StringView("chunked")) {
        reqIn = createChunkedInput(pool, conn->in);
    } else if (auto cl = headers.find(StringView("content-length")); cl) {
        reqIn = createLimitedInput(pool, conn->in, cl->stou());
    } else {
        reqIn = createZeroInput(pool);
    }
}

bool HttpServerRequestImpl::serve() {
    HttpServerResponseImpl resp(this);

    conn->opts->handler->serve(resp);
    reqIn->drain();

    return keepAlive;
}

StringView HttpServerRequestImpl::method() {
    return reqMethod;
}

StringView HttpServerRequestImpl::path() {
    return reqPath;
}

StringView HttpServerRequestImpl::query() {
    return reqQuery;
}

ZeroCopyInput* HttpServerRequestImpl::in() {
    return reqIn;
}

StringView* HttpServerRequestImpl::header(StringView name) {
    return headers.find(name);
}

HttpServerResponseImpl::HttpServerResponseImpl(HttpServerRequestImpl* req)
    : req(req)
    , rawOut(req->conn->out)
    , status(200)
{
}

Output* HttpServerResponseImpl::out() {
    return rawOut;
}

HttpServerRequest* HttpServerResponseImpl::request() {
    return req;
}

void HttpServerResponseImpl::setStatus(u32 code) {
    status = code;
}

void HttpServerResponseImpl::addHeader(StringView name, StringView value) {
    auto pool = req->pool;
    auto h = pool->make<Header>();

    h->name = pool->intern(name);
    h->value = pool->intern(value);

    headers.pushBack(h);
    headerIndex.insert(name.lower(req->conn->lcName), h);
}

void HttpServerResponseImpl::serialize(ZeroCopyOutput& out) {
    size_t need = 128;

    for (auto it = headers.begin(); it != headers.end(); ++it) {
        need += (*it)->name.length() + (*it)->value.length() + 4;
    }

    need += 2;

    auto start = out.imbue(need);
    auto buf = start;

    buf = buf << StringView(u8"HTTP/1.1 ")
              << (u64)status
              << StringView(u8" ")
              << reasonPhrase(status)
              << StringView(u8"\r\n");

    for (auto it = headers.begin(); it != headers.end(); ++it) {
        buf = buf << (*it)->name << StringView(u8": ") << (*it)->value << StringView(u8"\r\n");
    }

    buf = buf << StringView(u8"\r\n");

    out.commit(start.distance(buf));
}

void HttpServerResponseImpl::endHeaders() {
    auto pool = req->pool;

    auto cl = headerIndex.find(StringView("content-length"));
    auto te = headerIndex.find(StringView("transfer-encoding"));
    bool chunked = te && (*te)->value == StringView("chunked");

    if (req->keepAlive && !cl && !te) {
        addHeader(StringView("Transfer-Encoding"), StringView("chunked"));
        chunked = true;
    }

    if (auto zc = rawOut->upgrade()) {
        serialize(*zc);
    } else {
        StringBuilder sb;

        sb.xchg(req->conn->line);
        sb.reset();
        serialize(sb);
        rawOut->write(sb.data(), sb.used());
        sb.xchg(req->conn->line);
    }

    if (cl) {
        rawOut = createLimitedOutput(pool, rawOut, (*cl)->value.stou());
    } else if (chunked) {
        rawOut = createChunkedOutput(pool, rawOut);
    }

    if (auto conn = headerIndex.find(StringView("connection")); conn) {
        if ((*conn)->value != StringView("keep-alive")) {
            req->keepAlive = false;
        }
    }
}

HttpServerCtlImpl::HttpServerCtlImpl(HttpServeOpts* opts)
    : opts_(opts)
    , stopped(false)
{
}

void HttpServerCtlImpl::stop() {
    stdAtomicStore(&stopped, true, stl::MemoryOrder::Release);

    opts_->exec->spawn([this] {
        int cfd;

        if (TcpSocket::connectInf(&cfd, opts_->exec, opts_->addr, opts_->addrLen) == 0) {
            ::close(cfd);
        }
    });
}

TcpSocket* HttpServerCtlImpl::listen(ObjPool* pool) {
    int sfd;

    STD_VERIFY(TcpSocket::socket(&sfd, opts_->addr->sa_family, SOCK_STREAM, 0) == 0);

    auto srv = TcpSocket::create(pool, sfd, opts_->exec);

    srv->setReuseAddr(true);

    STD_VERIFY(srv->bind(opts_->addr, opts_->addrLen) == 0);
    STD_VERIFY(srv->listen(opts_->backlog) == 0);

    return srv;
}

void HttpServerCtlImpl::run(TcpSocket* srv) {
    for (;;) {
        auto cpool = ObjPool::fromMemory();
        auto client = cpool->make<ScopedFD>();

        if (srv->acceptInf(*client, nullptr, nullptr) != 0) {
            break;
        }

        if (stdAtomicFetch(&stopped, stl::MemoryOrder::Acquire)) {
            break;
        }

        opts_->exec->spawn([this, cpool, client] mutable {
            try {
                HttpConnection(opts_, cpool.mutPtr(), client).run();
            } catch (...) {
                sysE << Exception::current() << endL << flsH;
            }
        });
    }
}

HttpConnection::HttpConnection(HttpServeOpts* opts, ObjPool* pool, FD* client)
    : opts(opts)
    , pool(pool)
    , sock(client->get(), opts->exec)
{
    sock.setNoDelay(true);

    auto stream = pool->make<TcpStream>(sock);

    Input* in = stream;
    out = createOutBuf(pool, *stream);

    if (auto ssl = opts->handler->ssl()) {
        u8 b;

        if (sock.peek(b) && b == 0x16) {
            auto s = ssl->create(pool, in, out);

            in = s;
            out = s;
        }
    }

    this->in = createInBuf(pool, *in);
}

void HttpConnection::run() {
    while (serve()) {
        out->flush();
    }

    out->finish();
}

bool HttpConnection::serve() {
    StringView reqLine;

    if (!in->readLineZc(reqLine, line)) {
        return false;
    }

    auto pool = ObjPool::fromMemory();

    return HttpServerRequestImpl(pool.mutPtr(), this, reqLine).serve();
}

HttpServerCtl* stl::serve(ObjPool* pool, HttpServeOpts opts) {
    auto popts = internOpts(pool, opts);
    auto ctl = pool->make<HttpServerCtlImpl>(popts);
    auto srv = ctl->listen(pool);

    popts->wg->inc();

    popts->exec->spawn([popts, ctl, srv] {
        STD_DEFER {
            popts->wg->done();
        };

        ctl->run(srv);
    });

    return ctl;
}

SslCtx* HttpServe::ssl() {
    return nullptr;
}
