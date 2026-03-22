#include "http.h"

#include <std/thr/tcp.h>
#include <std/sys/crt.h>
#include <std/alg/minmax.h>
#include <std/thr/coro.h>
#include <std/alg/defer.h>
#include <std/ios/input.h>
#include <std/ios/in_zc.h>
#include <std/dbg/insist.h>
#include <std/dbg/verify.h>
#include <std/ios/in_buf.h>
#include <std/ios/output.h>
#include <std/lib/buffer.h>
#include <std/sys/atomic.h>
#include <std/mem/obj_pool.h>
#include <std/ios/stream_tcp.h>
#include <std/thr/wait_group.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace stl;

namespace {
    struct LimitedInput: public ZeroCopyInput {
        ZeroCopyInput* inner;
        size_t remaining;

        LimitedInput(ZeroCopyInput* inner, size_t limit);

        size_t readImpl(void* data, size_t len) override;
        size_t nextImpl(const void** chunk) override;
        void commitImpl(size_t len) noexcept override;
    };

    struct ChunkedInput: public ZeroCopyInput {
        ZeroCopyInput* inner;
        size_t chunkRemaining;
        bool eof;
        bool first;
        Buffer sizeBuf;

        ChunkedInput(ZeroCopyInput* inner);

        size_t readImpl(void* data, size_t len) override;
        size_t nextImpl(const void** chunk) override;
        void commitImpl(size_t len) noexcept override;

        bool loadChunk();
    };

    struct HttpServerCtlImpl: public HttpServerCtl {
        HttpServe& handler;
        CoroExecutor* exec;
        sockaddr_storage addr;
        u32 addrLen;
        WaitGroup& wg;
        bool stopped;

        HttpServerCtlImpl(HttpServe& handler, CoroExecutor* exec, const sockaddr* addr, u32 addrLen, WaitGroup& wg);

        void stop() override;
        void run();
        void acceptLoop(int srvFd);
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

LimitedInput::LimitedInput(ZeroCopyInput* inner, size_t limit)
    : inner(inner)
    , remaining(limit)
{
}

size_t LimitedInput::readImpl(void* data, size_t len) {
    len = min(len, remaining);

    if (!len) {
        return 0;
    }

    size_t n = inner->read(data, len);

    remaining -= n;

    return n;
}

size_t LimitedInput::nextImpl(const void** chunk) {
    if (!remaining) {
        return 0;
    }

    return min(inner->next(chunk), remaining);
}

void LimitedInput::commitImpl(size_t len) noexcept {
    inner->commit(len);
    remaining -= len;
}

ChunkedInput::ChunkedInput(ZeroCopyInput* inner)
    : inner(inner)
    , chunkRemaining(0)
    , eof(false)
    , first(true)
{
}

bool ChunkedInput::loadChunk() {
    if (!first) {
        char crlf[2];
        inner->read(crlf, 2);
    }

    first = false;
    sizeBuf.reset();
    inner->readLine(sizeBuf);
    chunkRemaining = StringView(sizeBuf).stripCr().stoh();

    if (!chunkRemaining) {
        eof = true;
        return false;
    }

    return true;
}

size_t ChunkedInput::readImpl(void* data, size_t len) {
    const void* chunk;

    len = min(next(&chunk), len);

    if (!len) {
        return 0;
    }

    memCpy(data, chunk, len);
    commit(len);

    return len;
}

size_t ChunkedInput::nextImpl(const void** chunk) {
    if (eof) {
        return 0;
    }

    if (!chunkRemaining && !loadChunk()) {
        return 0;
    }

    return min(inner->next(chunk), chunkRemaining);
}

void ChunkedInput::commitImpl(size_t len) noexcept {
    inner->commit(len);
    chunkRemaining -= len;
}

HttpServerCtlImpl::HttpServerCtlImpl(HttpServe& handler, CoroExecutor* exec, const sockaddr* addr, u32 addrLen, WaitGroup& wg)
    : handler(handler)
    , exec(exec)
    , addr{}
    , addrLen(addrLen)
    , wg(wg)
    , stopped(false)
{
    memCpy(&this->addr, addr, addrLen);
}

void HttpServerCtlImpl::stop() {
    stdAtomicStore(&stopped, true, stl::MemoryOrder::Release);

    int fd = ::socket(addr.ss_family, SOCK_STREAM, 0);

    if (fd >= 0) {
        ::connect(fd, (const sockaddr*)&addr, addrLen);
        ::close(fd);
    }
}

void HttpServerCtlImpl::run() {
    TcpSocket srv(exec);

    STD_VERIFY(srv.socket(AF_INET, SOCK_STREAM, 0) == 0);

    srv.setReuseAddr(true);

    STD_VERIFY(srv.bind((const sockaddr*)&addr, addrLen) == 0);
    STD_VERIFY(srv.listen(128) == 0);

    wg.inc();

    exec->spawn([this, srvFd = srv.fd] {
        acceptLoop(srvFd);
    });
}

void HttpServerCtlImpl::acceptLoop(int srvFd) {
    TcpSocket srv(srvFd, exec);

    STD_DEFER {
        srv.close();
        wg.done();
    };

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
    req.path = pool->intern(path.empty() ? rest : path);

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
        req.in = pool->make<ChunkedInput>(&buf);
    } else if (auto cl = req.headers.find(StringView("content-length")); cl) {
        req.in = pool->make<LimitedInput>(&buf, cl->stou());
    } else {
        req.in = nullptr;
    }

    handler.serve(req);

    if (req.in) {
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
    auto ctl = makeIntrusivePtr(new HttpServerCtlImpl(handler, exec, addr, addrLen, wg));

    ctl->run();

    return ctl.mutPtr();
}

HttpServerCtl::~HttpServerCtl() {
}
