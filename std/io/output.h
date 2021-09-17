#pragma once

#include <std/os/types.h>

#define stdE (::Std::stderrStream())
#define stdO (::Std::stdoutStream())
#define endL (::Std::EndLine())

namespace Std {
    struct EndLine {
    };

    struct Output;

    template <typename T>
    void output(Output& out, const T& t);

    struct Output {
        virtual ~Output();

        inline void write(const void* data, size_t len) {
            if (len) {
                writeImpl(data, len);
            }
        }

        inline void write(u8 ch) {
            write(&ch, 1);
        }

        template <typename Out, typename T>
        friend inline Out& operator<<(Out& out, const T& t) {
            output(out, t);

            return out;
        }

        inline void flush() {
            flushImpl();
        }

        inline void finish() {
            finishImpl();
        }

    private:
        virtual void writeImpl(const void* data, size_t len) = 0;

        // have sensible defaults
        virtual void flushImpl();
        virtual void finishImpl();
    };

    Output& stdoutStream() noexcept;
    Output& stderrStream() noexcept;
}
