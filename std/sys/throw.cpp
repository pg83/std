#include "throw.h"

#include <std/str/view.h>
#include <std/lib/buffer.h>
#include <std/typ/support.h>
#include <std/str/builder.h>

#include <errno.h>
#include <string.h>

using namespace stl;

namespace {
    struct ErrnoError: public Exception {
        Buffer full;
        Buffer text;
        int error;

        ErrnoError(int e, Buffer&& t)
            : text(move(t))
            , error(e)
        {
        }

        ExceptionKind kind() const override {
            return ExceptionKind::Errno;
        }

        StringView description() override {
            if (!full.empty()) {
                return full;
            }

            (StringBuilder()
             << StringView(u8"(code ")
             << error
             << StringView(u8", descr ")
             << (const char*)strerror(error)
             << StringView(u8") ")
             << text)
                .xchg(full);

            return full;
        }
    };
}

Exception::~Exception() {
}

StringView Exception::current() {
    try {
        throw;
    } catch (Exception& e) {
        return e.description();
    } catch (...) {
        return StringView(u8"unknown exception");
    }
}

Errno::Errno()
    : error(errno)
{
    errno = 0;
}

void Errno::raise(Buffer&& text) {
    throw ErrnoError(error, move(text));
}
