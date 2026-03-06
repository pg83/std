#include "manip.h"
#include "out_zc.h"

using namespace stl;
using namespace stl::Manip;

// modifiers
template <>
void stl::output<ZeroCopyOutput, Flush>(ZeroCopyOutput& out, Flush) {
    out.flush();
}

template <>
void stl::output<ZeroCopyOutput, Finish>(ZeroCopyOutput& out, Finish) {
    out.finish();
}

template <>
void stl::output<ZeroCopyOutput, EndLine>(ZeroCopyOutput& out, EndLine) {
    out.write(u8"\n", 1);
}
