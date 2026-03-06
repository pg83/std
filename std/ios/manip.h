#pragma once

#define flsH (::stl::Manip::Flush())
#define finI (::stl::Manip::Finish())
#define endL (::stl::Manip::EndLine())

namespace stl::Manip {
    struct EndLine {
    };

    struct Flush {
    };

    struct Finish {
    };
}
