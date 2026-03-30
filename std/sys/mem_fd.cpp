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

namespace {
#if !defined(__linux__)
    int memFDFallback(const char* name) {
        (void)name;

        char path[] = "/tmp/memfd.XXXXXX";
        int fd = mkstemp(path);

        if (fd < 0) {
            stl::Errno().raise(stl::StringBuilder() << stl::StringView(u8"mkstemp() failed"));
        }

        unlink(path);

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
