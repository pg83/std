#pragma once

#include <std/str/view.h>
#include <std/sys/types.h>

struct sockaddr;

namespace stl {
    class Input;
    class Output;
    class ObjPool;
    class WaitGroup;
    class ZeroCopyInput;

    struct SslCtx;
    struct CoroExecutor;

    struct HttpServerRequest {
        virtual StringView path() = 0;
        virtual StringView query() = 0;
        virtual StringView method() = 0;
        virtual ZeroCopyInput* in() = 0;
        virtual StringView* header(StringView name) = 0;
    };

    struct HttpServerResponse {
        virtual Output* out() = 0;
        virtual void endHeaders() = 0;
        virtual void setStatus(u32 code) = 0;
        virtual HttpServerRequest* request() = 0;
        virtual void addHeader(StringView name, StringView value) = 0;
    };

    struct HttpServe {
        virtual SslCtx* ssl();
        virtual void serve(HttpServerResponse& resp) = 0;
    };

    struct HttpServerCtl {
        virtual ~HttpServerCtl();

        virtual void stop() = 0;
    };

    struct HttpServeOpts {
        HttpServe* handler = nullptr;
        CoroExecutor* exec = nullptr;
        const sockaddr* addr = nullptr;
        WaitGroup* wg = nullptr;
        u32 addrLen = 0;
        u32 backlog = 128;
    };

    HttpServerCtl* serve(ObjPool* pool, HttpServeOpts opts);
}
