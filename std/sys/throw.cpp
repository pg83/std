#include "throw.h"

#include <std/str/view.h>
#include <std/lib/buffer.h>
#include <std/typ/support.h>
#include <std/str/builder.h>

#include <string.h>

using namespace Std;

namespace {
    struct Errno: public Exception {
        Buffer full;
        Buffer text;
        int error;

        inline Errno(int e, Buffer&& t) noexcept
            : text(move(t))
            , error(e)
        {
        }

        ExceptionKind kind() const noexcept override {
            return ExceptionKind::Errno;
        }

        StringView description() override {
            (StringBuilder()
                << StringView(u8"(code ")
                << error
                << StringView(u8", descr ")
                << (const char*)strerror(error)
                << StringView(u8") ")
                << text).xchg(full);

            return full;
        }
    };
}

Exception::~Exception() noexcept {
}

void Std::throwErrno(int error, Buffer&& text) {
    throw Errno(error, move(text));
}
