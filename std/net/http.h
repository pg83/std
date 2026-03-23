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

    struct CoroExecutor;

    struct HttpRequest {
        ObjPool* opool;
        StringView method;
        StringView path;
        StringView query;
        SymbolMap<StringView> headers;
        ZeroCopyInput* in;
        Output* out;
    };

    struct HttpServe {
        virtual void serve(HttpRequest& req) = 0;
    };

    struct HttpServerCtl: public ARC {
        virtual ~HttpServerCtl();

        virtual void stop() = 0;
    };

    struct SslCtx;

    IntrusivePtr<HttpServerCtl> serve(HttpServe& handler, CoroExecutor* exec, const sockaddr* addr, u32 addrLen, WaitGroup& wg, SslCtx* ssl = nullptr);
}
