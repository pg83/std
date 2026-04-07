#include "mem_fd.h"

#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/str/builder.h>

#if defined(__linux__)
    #include <sys/mman.h>
#else
    #include <unistd.h>
    #include <stdlib.h>
#endif

using namespace stl;

namespace {
#if !defined(__linux__)
    int memFDFallback(const char* name) {
        (void)name;

        const char* dir = getenv("TMPDIR");

        if (!dir) {
            dir = "/tmp";
        }

        StringBuilder sb;
        sb << StringView(dir) << StringView(u8"/memfd.XXXXXX");

        int fd = mkstemp(sb.cStr());

        if (fd < 0) {
            Errno().raise(StringBuilder() << StringView(u8"mkstemp() failed"));
        }

        unlink(sb.cStr());

        return fd;
    }
#endif
}

int stl::memFD(const char* name) {
#if defined(__linux__)
    int fd = memfd_create(name, 0);

    if (fd < 0) {
        Errno().raise(StringBuilder() << StringView(u8"memfd_create() failed"));
    }

    return fd;
#else
    return memFDFallback(name);
#endif
}
