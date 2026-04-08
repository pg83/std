#include "http_client.h"

#include "http_io.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/ios/in_zc.h>
#include <std/ios/output.h>
#include <std/sym/s_map.h>
#include <std/lib/buffer.h>
#include <std/lib/vector.h>
#include <std/ios/in.h>
#include <std/str/builder.h>
#include <std/mem/obj_pool.h>

using namespace stl;

namespace {
    struct HttpClientResponseImpl: public HttpClientResponse {
        ObjPool* pool;
        u32 statusCode;
        StringView statusReason;
        SymbolMap<StringView> headers;
        ZeroCopyInput* bodyIn;
        Buffer line;
        Buffer lcName;

        HttpClientResponseImpl(ObjPool* pool, ZeroCopyInput* in);

        u32 status() override;
        StringView reason() override;
        StringView* header(StringView name) override;
        ZeroCopyInput* body() override;
    };

    struct HttpClientRequestImpl: public HttpClientRequest {
        struct Header {
            StringView name;
            StringView value;
        };

        ObjPool* pool;
        ZeroCopyInput* in;
        Output* rawOut;
        StringView reqMethod;
        StringView reqPath;
        Vector<Header*> headers;
        SymbolMap<Header*> headerIndex;
        Buffer line;
        Buffer lcName;

        HttpClientRequestImpl(ObjPool* pool, ZeroCopyInput* in, Output* out);

        void setMethod(StringView method) override;
        void setPath(StringView path) override;
        void addHeader(StringView name, StringView value) override;
        void endHeaders() override;
        Output* out() override;
        HttpClientResponse* response() override;

        void serialize(StringBuilder& sb);
    };
}

HttpClientResponseImpl::HttpClientResponseImpl(ObjPool* pool, ZeroCopyInput* in)
    : pool(pool)
    , statusCode(0)
    , headers(pool)
    , bodyIn(nullptr)
{
    in->readLine(line);

    StringView first(line);

    first = first.stripCr();

    StringView version, rest, code, reason;

    first.split(' ', version, rest);
    rest.split(' ', code, reason);

    statusCode = code.stou();
    statusReason = pool->intern(reason);

    for (;;) {
        line.reset();
        in->readLine(line);

        StringView name, val;

        if (!StringView(line).stripCr().split(':', name, val)) {
            break;
        }

        headers.insert(name.lower(lcName), pool->intern(val.stripSpace()));
    }

    if (auto te = headers.find(StringView("transfer-encoding")); te && te->lower(line) == StringView("chunked")) {
        bodyIn = createChunkedInput(pool, in);
    } else if (auto cl = headers.find(StringView("content-length")); cl) {
        bodyIn = createLimitedInput(pool, in, cl->stou());
    } else {
        bodyIn = createZeroInput(pool);
    }
}

u32 HttpClientResponseImpl::status() {
    return statusCode;
}

StringView HttpClientResponseImpl::reason() {
    return statusReason;
}

StringView* HttpClientResponseImpl::header(StringView name) {
    return headers.find(name.lower(lcName));
}

ZeroCopyInput* HttpClientResponseImpl::body() {
    return bodyIn;
}

HttpClientRequestImpl::HttpClientRequestImpl(ObjPool* pool, ZeroCopyInput* in, Output* out)
    : pool(pool)
    , in(in)
    , rawOut(out)
    , reqMethod(StringView("GET"))
    , reqPath(StringView("/"))
    , headerIndex(pool)
{
}

void HttpClientRequestImpl::setMethod(StringView method) {
    reqMethod = pool->intern(method);
}

void HttpClientRequestImpl::setPath(StringView path) {
    reqPath = pool->intern(path);
}

void HttpClientRequestImpl::addHeader(StringView name, StringView value) {
    auto h = pool->make<Header>();

    h->name = pool->intern(name);
    h->value = pool->intern(value);

    headers.pushBack(h);
    headerIndex.insert(name.lower(lcName), h);
}

void HttpClientRequestImpl::serialize(StringBuilder& sb) {
    sb << reqMethod
       << StringView(u8" ")
       << reqPath
       << StringView(u8" HTTP/1.1\r\n");

    for (auto it = headers.begin(); it != headers.end(); ++it) {
        sb << (*it)->name << StringView(u8": ") << (*it)->value << StringView(u8"\r\n");
    }

    sb << StringView(u8"\r\n");
}

void HttpClientRequestImpl::endHeaders() {
    if (!headerIndex.find(StringView("content-length")) && !headerIndex.find(StringView("transfer-encoding"))) {
        if (reqMethod == StringView("POST") || reqMethod == StringView("PUT") || reqMethod == StringView("PATCH")) {
            addHeader(StringView("Transfer-Encoding"), StringView("chunked"));
        }
    }

    {
        StringBuilder sb;

        serialize(sb);
        rawOut->write(sb.data(), sb.used());
    }

    if (auto cl = headerIndex.find(StringView("content-length")); cl) {
        rawOut = createLimitedOutput(pool, rawOut, (*cl)->value.stou());
    } else if (auto te = headerIndex.find(StringView("transfer-encoding")); te && (*te)->value == StringView("chunked")) {
        rawOut = createChunkedOutput(pool, rawOut);
    }
}

Output* HttpClientRequestImpl::out() {
    return rawOut;
}

HttpClientResponse* HttpClientRequestImpl::response() {
    rawOut->finish();
    return pool->make<HttpClientResponseImpl>(pool, in);
}

HttpClientRequest* stl::HttpClientRequest::create(ObjPool* pool, ZeroCopyInput* in, Output* out) {
    return pool->make<HttpClientRequestImpl>(pool, in, out);
}
