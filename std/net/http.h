#pragma once

#include <std/str/view.h>
#include <std/sym/s_map.h>
#include <std/sys/types.h>

struct sockaddr;

namespace stl {
    class Input;
    class Output;

    struct CoroExecutor;

    struct HttpRequest {
        StringView method;
        StringView path;
        SymbolMap<StringView> headers;
        Input& in;
        Output& out;

        HttpRequest(Input& in, Output& out) noexcept;
    };

    struct HttpServe {
        virtual void serve(HttpRequest& req) = 0;
    };

    void serve(HttpServe& handler, CoroExecutor* exec, const sockaddr* addr, u32 addrLen);
}
