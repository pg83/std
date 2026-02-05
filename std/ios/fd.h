#pragma once

#include "output.h"

namespace Std {
    class FDOutput: public Output {
        size_t writeImpl(const void* data, size_t len) override;
        size_t writeVImpl(iovec* parts, size_t count) override;

        void finishImpl() override;

    public:
        // does not own fd
        int fd;

        inline FDOutput(int _fd) noexcept
            : fd(_fd)
        {
        }

        ~FDOutput() noexcept override;
    };

    class FDRegular: public FDOutput {
        void flushImpl() override;
        size_t hintImpl() const noexcept override;

    public:
        FDRegular(int fd) noexcept;
    };

    class FDCharacter: public FDOutput {
        size_t hintImpl() const noexcept override;

    public:
        FDCharacter(int fd) noexcept;
    };

    class FDPipe: public FDOutput {
        size_t hintImpl() const noexcept override;

    public:
        FDPipe(int fd) noexcept;
    };
}
