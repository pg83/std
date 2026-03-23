#pragma once

#include "output.h"

#include <std/sys/types.h>

namespace stl {
    class FD;

    struct CoroExecutor;

    class CoroFDOutput: public Output {
        size_t writeImpl(const void* data, size_t len) override;
        size_t hintImpl() const noexcept override;

    public:
        FD* fd;
        CoroExecutor* exec;
        off_t offset;

        CoroFDOutput(FD& fd, CoroExecutor* exec) noexcept;
        ~CoroFDOutput() noexcept override;
    };
}
