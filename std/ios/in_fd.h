#pragma once

#include "input.h"

namespace Std {
    class FD;

    class FDInput: public Input {
        size_t readImpl(void* data, size_t len) override;
        size_t hintImpl() const noexcept override;

    public:
        FD* fd;

        inline FDInput(FD& _fd) noexcept
            : fd(&_fd)
        {
        }

        ~FDInput() noexcept override;
    };
}
