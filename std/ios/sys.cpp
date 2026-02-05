#include "fd.h"
#include "sys.h"
#include "output.h"

#include <sys/stat.h>

using namespace Std;

namespace {
    static inline FDOutput* wrap(int fd) {
        struct stat st;

        if (fstat(fd, &st) == 0) {
            if (S_ISREG(st.st_mode) || S_ISBLK(st.st_mode)) {
                return new FDRegular(fd);
            }

            if (S_ISCHR(st.st_mode)) {
                return new FDCharacter(fd);
            }
        }

        return new FDPipe(fd);
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
