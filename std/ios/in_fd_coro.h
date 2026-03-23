#pragma once

#include "input.h"

#include <std/sys/types.h>

namespace stl {
    class FD;

    struct CoroExecutor;

    class CoroFDInput: public Input {
        size_t readImpl(void* data, size_t len) override;
        size_t hintImpl() const noexcept override;

    public:
        FD* fd;
        CoroExecutor* exec;
        off_t offset;

        CoroFDInput(FD& fd, CoroExecutor* exec) noexcept;
        ~CoroFDInput() noexcept override;
    };
}
