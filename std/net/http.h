#pragma once

#include <std/ptr/arc.h>
#include <std/str/view.h>
#include <std/sym/s_map.h>
#include <std/sys/types.h>
#include <std/ptr/intrusive.h>

struct sockaddr;

namespace stl {
    class Input;
    class Output;
    class ObjPool;
    class WaitGroup;
    class ZeroCopyInput;

    struct SslCtx;
    struct CoroExecutor;
    struct HttpResponse;

    struct HttpRequest {
        ObjPool* opool;
        StringView method;
        StringView path;
        StringView query;
        SymbolMap<StringView> headers;
        ZeroCopyInput* in;
        Output* out;
        bool keepAlive;
    };

    struct HttpResponse {
        virtual Output* out() = 0;
        virtual void endHeaders() = 0;
        virtual HttpRequest* request() = 0;
        virtual void setStatus(u32 code) = 0;
        virtual void addHeader(StringView name, StringView value) = 0;
    };

    struct HttpServe {
        virtual SslCtx* ssl();
        virtual void serve(HttpResponse& resp) = 0;
    };

    struct HttpServerCtl: public ARC {
        virtual ~HttpServerCtl();

        virtual void stop() = 0;
    };

    IntrusivePtr<HttpServerCtl> serve(HttpServe& handler, CoroExecutor* exec, const sockaddr* addr, u32 addrLen, WaitGroup& wg);
}
