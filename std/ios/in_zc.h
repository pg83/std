#pragma once

#include "input.h"

#include <std/sys/types.h>

namespace Std {
    class ZeroCopyInput: public Input {
        size_t readImpl(void* data, size_t len) override;

        virtual size_t nextImpl(const void** chunk) = 0;
        virtual void commitImpl(size_t len) noexcept = 0;

    public:
        ~ZeroCopyInput() noexcept override;

        inline size_t next(const void** chunk) {
            return nextImpl(chunk);
        }

        inline void commit(size_t len) noexcept {
            commitImpl(len);
        }

        ZeroCopyInput* zeroCopy() noexcept override;
    };
}
