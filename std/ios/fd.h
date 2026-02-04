#pragma once

#include "output.h"

namespace Std {
    class FDOutput: public Output {
        // does not own fd
        int fd;

        size_t writeImpl(const void* data, size_t len) override;
        size_t writeVImpl(iovec* parts, size_t count) override;
        size_t hintImpl() const noexcept override;

        void flushImpl() override;
        void finishImpl() override;

    public:
        inline FDOutput(int _fd) noexcept
            : fd(_fd)
        {
        }

        ~FDOutput() noexcept override;
    };
}
