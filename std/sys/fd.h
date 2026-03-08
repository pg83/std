#pragma once

#include <std/sys/types.h>

struct iovec;

namespace stl {
    class FD {
        // does not own fd
        int fd;

    public:
        FD()
            : fd(-1)
        {
        }

        FD(int _fd)
            : fd(_fd)
        {
        }

        size_t read(void* data, size_t len);
        size_t writeV(iovec* parts, size_t count);
        size_t write(const void* data, size_t len);

        void close();
        void fsync();

        void xchg(FD& fd);
    };

    struct ScopedFD: public FD {
        using FD::FD;

        ~ScopedFD() noexcept(false);
    };

    void createPipeFD(ScopedFD& in, ScopedFD& out);
}
