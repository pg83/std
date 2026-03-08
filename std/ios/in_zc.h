#pragma once

#include "input.h"

#include <std/sys/types.h>

namespace stl {
    class Buffer;

    class ZeroCopyInput: public Input {
        void sendTo(Output& out) override;
        size_t readImpl(void* data, size_t len) override;

        virtual size_t nextImpl(const void** chunk) = 0;
        virtual void commitImpl(size_t len) noexcept = 0;

    public:
        ~ZeroCopyInput() noexcept override;

        bool readLine(Buffer& buf);
        bool readTo(Buffer& buf, u8 delim);

        size_t next(const void** chunk) {
            return nextImpl(chunk);
        }

        void commit(size_t len) noexcept {
            commitImpl(len);
        }
    };
}
