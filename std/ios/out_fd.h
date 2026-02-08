#pragma once

#include "output.h"

namespace Std {
    class FD;

    class FDOutput: public Output {
        size_t writeImpl(const void* data, size_t len) override;
        size_t writeVImpl(iovec* parts, size_t count) override;

        void finishImpl() override;

    public:
        // does not own fd
        FD* fd;

        inline FDOutput(FD& _fd) noexcept
            : fd(&_fd)
        {
        }

        ~FDOutput() noexcept override;
    };

    class FDRegular: public FDOutput {
        void flushImpl() override;
        size_t hintImpl() const noexcept override;

    public:
        FDRegular(FD& fd) noexcept;
    };

    class FDCharacter: public FDOutput {
        size_t hintImpl() const noexcept override;

    public:
        FDCharacter(FD& fd) noexcept;
    };

    class FDPipe: public FDOutput {
        size_t hintImpl() const noexcept override;

    public:
        FDPipe(FD& fd) noexcept;
    };
}
