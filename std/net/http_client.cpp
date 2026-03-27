#include "http_client.h"

#include "http_io.h"

#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/ios/in_zc.h>
#include <std/sym/s_map.h>
#include <std/lib/buffer.h>
#include <std/ios/in_zero.h>
#include <std/mem/obj_pool.h>

using namespace stl;

namespace {
    struct HttpClientImpl: public HttpClient {
        ObjPool* pool;
        u32 statusCode;
        StringView statusReason;
        SymbolMap<StringView> headers;
        ZeroCopyInput* bodyIn;
        Buffer line;
        Buffer lcName;

        HttpClientImpl(ObjPool* pool, ZeroCopyInput* in);

        u32 status() override;
        StringView reason() override;
        StringView* header(StringView name) override;
        ZeroCopyInput* body() override;
    };
}

HttpClientImpl::HttpClientImpl(ObjPool* pool, ZeroCopyInput* in)
    : pool(pool)
    , statusCode(0)
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
        bodyIn = pool->make<ZeroInput>();
    }
}

u32 HttpClientImpl::status() {
    return statusCode;
}

StringView HttpClientImpl::reason() {
    return statusReason;
}

StringView* HttpClientImpl::header(StringView name) {
    return headers.find(name.lower(lcName));
}

ZeroCopyInput* HttpClientImpl::body() {
    return bodyIn;
}

HttpClient* stl::HttpClient::create(ObjPool* pool, ZeroCopyInput* in) {
    return pool->make<HttpClientImpl>(pool, in);
}
