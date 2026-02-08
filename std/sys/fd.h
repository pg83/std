#pragma once

#include <std/sys/types.h>

struct iovec;

namespace Std {
    class FD {
        // does not own fd
        int fd;

    public:
        inline FD(int _fd) noexcept
            : fd(_fd)
        {
        }

        size_t read(void* data, size_t len);
        size_t writeV(iovec* parts, size_t count);
        size_t write(const void* data, size_t len);

        void close();
        void fsync();

        void xchg(FD& fd) noexcept;
    };

    struct ScopedFD: public FD {
        using FD::FD;

        inline ~ScopedFD() {
            close();
        }
    };

    void createPipeFD(ScopedFD& in, ScopedFD& out);
}
