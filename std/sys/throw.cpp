#include "throw.h"

#include <std/lib/buffer.h>
#include <std/typ/support.h>

using namespace Std;

namespace {
    struct Errno: public Exception {
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
    };
}

Exception::~Exception() noexcept {
}

void Std::throwErrno(int error, Buffer&& text) {
    throw Errno(error, move(text));
}
