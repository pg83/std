#pragma once

#include "input.h"

#include <std/sys/types.h>

namespace Std {
    class ZeroCopyInput: public Input {
        size_t readImpl(void* data, size_t len) override;

        virtual const void* nextImpl(size_t* len) = 0;
        virtual void commitImpl(size_t len) noexcept = 0;

    public:
        ~ZeroCopyInput() noexcept override;

        inline const void* next(size_t* len) {
            return nextImpl(len);
        }

        const void* nextLimited(size_t* len);

        inline void commit(size_t len) noexcept {
            commitImpl(len);
        }

        ZeroCopyInput* zeroCopy() noexcept override;
    };
}
