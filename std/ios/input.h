#pragma once

#include <std/sys/types.h>

namespace Std {
    class Input {
        virtual size_t readImpl(void* data, size_t len) = 0;

    public:
        inline size_t readP(void* data, size_t len) {
            return readImpl(data, len);
        }

        void read(void* data, size_t len);
    };
}
