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
    class WaitGroup;
    class ZeroCopyInput;

    struct CoroExecutor;

    struct HttpRequest {
        StringView method;
        StringView path;
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

    IntrusivePtr<HttpServerCtl> serve(HttpServe& handler, CoroExecutor* exec, const sockaddr* addr, u32 addrLen, WaitGroup& wg);
}
