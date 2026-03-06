#pragma once

#include <std/sys/types.h>

namespace stl {
    class Output;
    class Buffer;
    class ZeroCopyInput;

    class Input {
        virtual size_t hintImpl() const noexcept;
        virtual size_t readImpl(void* data, size_t len) = 0;

    public:
        virtual ~Input() noexcept;

        inline size_t read(void* data, size_t len) {
            return readImpl(data, len);
        }

        inline size_t hint() const noexcept {
            return hintImpl();
        }

        inline size_t hint(size_t def) const noexcept {
            if (auto h = hint(); h) {
                return h;
            }

            return def;
        }

        void readAll(Buffer& res);

        virtual void sendTo(Output& out);
    };
}
