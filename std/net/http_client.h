#pragma once

#include <std/str/view.h>

namespace stl {
    class ObjPool;
    class ZeroCopyInput;

    struct HttpClient {
        virtual u32 status() = 0;
        virtual StringView reason() = 0;
        virtual StringView* header(StringView name) = 0;
        virtual ZeroCopyInput* body() = 0;

        static HttpClient* create(ObjPool* pool, ZeroCopyInput* in);
    };
}
