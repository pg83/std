#include "fs_utils.h"

#include "in_fd.h"

#include <std/sys/fd.h>
#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/lib/buffer.h>
#include <std/str/builder.h>

#include <fcntl.h>
#include <unistd.h>

using namespace stl;

void stl::readFileContent(Buffer& path, Buffer& out) {
    int rawFd = ::open(path.cStr(), O_RDONLY);

    if (rawFd < 0) {
        Errno().raise(StringBuilder() << StringView(u8"can not open ") << path);
    }

    ScopedFD fd(rawFd);
    FDInput(fd).readAll(out);
}
