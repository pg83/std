#pragma once

#include "input.h"

namespace stl {
    class FD;

    class FDInput: public Input {
        size_t readImpl(void* data, size_t len) override;
        size_t hintImpl() const override;

    public:
        FD* fd;

        FDInput(FD& _fd)
            : fd(&_fd)
        {
        }

        ~FDInput() override;
    };
}
