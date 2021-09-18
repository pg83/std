#pragma once

#include "buf.h"
#include "manip.h"
#include "output.h"

#define sysO (::Std::OutBuf(::Std::stdoutStream()))
#define sysE (::Std::OutBuf(::Std::stderrStream()))

namespace Std {
    Output& stdoutStream() noexcept;
    Output& stderrStream() noexcept;
}
