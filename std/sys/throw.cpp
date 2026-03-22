#include "throw.h"

#include <std/str/view.h>
#include <std/lib/buffer.h>
#include <std/typ/support.h>
#include <std/str/builder.h>

#include <errno.h>
#include <string.h>

using namespace stl;

namespace {
    struct VerifyError: public Exception {
        const u8* what_;
        u32 line_;
        const u8* file_;
        Buffer full;

        VerifyError(const u8* what, u32 line, const u8* file) noexcept
            : what_(what)
            , line_(line)
            , file_(file)
        {
        }

        ExceptionKind kind() const noexcept override {
            return ExceptionKind::Verify;
        }

        StringView description() override {
            if (!full.empty()) {
                return full;
            }

            (StringBuilder()
             << StringView((const char*)file_)
             << StringView(u8":")
             << line_
             << StringView(u8": verify failed: ")
             << StringView((const char*)what_))
                .xchg(full);

            return full;
        }
    };

    struct ErrnoError: public Exception {
        Buffer full;
        Buffer text;
        int error;

        ErrnoError(int e, Buffer&& t) noexcept
            : text(move(t))
            , error(e)
        {
        }

        ExceptionKind kind() const noexcept override {
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

void stl::raiseVerify(const u8* what, u32 line, const u8* file) {
    throw VerifyError(what, line, file);
}

Exception::~Exception() noexcept {
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

Errno::Errno() noexcept
    : error(errno)
{
    errno = 0;
}

Errno::Errno(int error) noexcept
    : error(error)
{
}

void Errno::raise(Buffer&& text) {
    throw ErrnoError(error, move(text));
}
