#pragma once

#include <std/ios/input.h>
#include <std/ios/output.h>

namespace stl {
    class ObjPool;
    class StringView;

    struct SslSocket: public Input, public Output {
    };

    struct SslCtx {
        virtual SslSocket* create(ObjPool* pool, Input* in, Output* out) = 0;

        static SslCtx* create(ObjPool* pool, StringView cert, StringView key);
    };
}
