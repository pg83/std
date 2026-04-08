#pragma once

#include <std/str/view.h>

namespace stl {
    class Output;
    class ObjPool;
    class ZeroCopyInput;

    struct HttpClientResponse {
        virtual u32 status() = 0;
        virtual StringView reason() = 0;
        virtual ZeroCopyInput* body() = 0;
        virtual StringView* header(StringView name) = 0;
    };

    struct HttpClientRequest {
        virtual void setMethod(StringView method) = 0;
        virtual void setPath(StringView path) = 0;
        virtual void addHeader(StringView name, StringView value) = 0;
        virtual void endHeaders() = 0;
        virtual Output* out() = 0;
        virtual HttpClientResponse* response() = 0;

        static HttpClientRequest* create(ObjPool* pool, ZeroCopyInput* in, Output* out);
    };
}
