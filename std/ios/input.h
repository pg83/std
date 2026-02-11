#pragma once

#include <std/sys/types.h>

namespace Std {
    class Output;
    class Buffer;
    class ZeroCopyInput;

    class Input {
        virtual size_t readImpl(void* data, size_t len) = 0;

    public:
        virtual ~Input() noexcept;

        inline size_t read(void* data, size_t len) {
            return readImpl(data, len);
        }

        void readAll(Buffer& res);

        virtual void sendTo(Output& out);
    };
}
