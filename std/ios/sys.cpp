#include "sys.h"
#include "out_fd.h"
#include "output.h"

#include <std/sys/fd.h>

#include <sys/stat.h>

using namespace Std;

namespace {
    template <typename T>
    struct Scoped: public FD, public T {
        inline Scoped(int fd) noexcept
            : FD(fd)
            , T(*(FD*)this)
        {
        }
    };

    static inline FDOutput* wrap(int fd) {
        struct stat st;

        if (fstat(fd, &st) == 0) {
            if (S_ISREG(st.st_mode) || S_ISBLK(st.st_mode)) {
                return new Scoped<FDRegular>(fd);
            }

            if (S_ISCHR(st.st_mode)) {
                return new Scoped<FDCharacter>(fd);
            }
        }

        return new Scoped<FDPipe>(fd);
    }
}

Output& Std::stdoutStream() noexcept {
    static auto fd = wrap(1);

    return *fd;
}

Output& Std::stderrStream() noexcept {
    static auto fd = wrap(2);

    return *fd;
}
