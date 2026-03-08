#pragma once

#include "manip.h"
#include "output.h"
#include "out_buf.h"

#define sysO ::stl::OutBuf(::stl::stdoutStream())
#define sysE ::stl::OutBuf(::stl::stderrStream())

namespace stl {
    Output& stdoutStream() noexcept;
    Output& stderrStream() noexcept;
}
