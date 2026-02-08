#pragma once

#include "manip.h"
#include "output.h"
#include "out_buf.h"

#define sysO ::Std::OutBuf(::Std::stdoutStream())
#define sysE ::Std::OutBuf(::Std::stderrStream())

namespace Std {
    Output& stdoutStream() noexcept;
    Output& stderrStream() noexcept;
}
