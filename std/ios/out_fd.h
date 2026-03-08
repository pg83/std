#pragma once

#include "output.h"

namespace stl {
    class FD;

    class FDOutput: public Output {
        size_t writeImpl(const void* data, size_t len) override;
        size_t writeVImpl(iovec* parts, size_t count) override;

        void finishImpl() override;

    public:
        // does not own fd
        FD* fd;

        FDOutput(FD& _fd)
            : fd(&_fd)
        {
        }

        ~FDOutput() override;
    };

    class FDRegular: public FDOutput {
        void flushImpl() override;
        size_t hintImpl() const override;

    public:
        FDRegular(FD& fd);
    };

    class FDCharacter: public FDOutput {
        size_t hintImpl() const override;

    public:
        FDCharacter(FD& fd);
    };

    class FDPipe: public FDOutput {
        size_t hintImpl() const override;

    public:
        FDPipe(FD& fd);
    };
}
