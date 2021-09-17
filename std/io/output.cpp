#include "output.h"

#include <std/tl/singleton.h>

#include <stdio.h>

using namespace Std;

namespace {
    struct StdOutput: public Output {
        inline StdOutput(FILE* s) noexcept
            : stream(s)
        {
        }

        void writeImpl(const void* data, size_t len) override {
            fwrite(data, 1, len, stream);
        }

        void flushImpl() override {
            fflush(stream);
        }

        FILE* stream;
    };

    struct StdErr: public StdOutput {
        inline StdErr() noexcept
            : StdOutput(stderr)
        {
        }
    };

    struct StdOut: public StdOutput {
        inline StdOut() noexcept
            : StdOutput(stdout)
        {
        }
    };
}

Output::~Output() {
}

void Output::flushImpl() {
}

void Output::finishImpl() {
}

Output& Std::stdoutStream() noexcept {
    return singleton<StdOut>();
}

Output& Std::stderrStream() noexcept {
    return singleton<StdErr>();
}

template <>
void Std::output<EndLineFunc>(Output& out, const EndLineFunc&) {
    out << u8'\n' << flsH;
}

template <>
void Std::output<FlushFunc>(Output& out, const FlushFunc&) {
    out.flush();
}

template <>
void Std::output<c8>(Output& out, const c8& ch) {
    out.write(&ch, 1);
}

#include <string.h>

template <>
void Std::output<u64>(Output& out, const u64& v) {
    char buf[100];

    sprintf(buf, "%u", (unsigned)v);
    out.write(buf, strlen(buf));
}
