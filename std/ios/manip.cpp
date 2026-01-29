#include "manip.h"
#include "zc_out.h"

using namespace Std;
using namespace Std::Manip;

// modifiers
template <>
void Std::output<ZeroCopyOutput, Flush>(ZeroCopyOutput& out, Flush) {
    out.flush();
}

template <>
void Std::output<ZeroCopyOutput, Finish>(ZeroCopyOutput& out, Finish) {
    out.finish();
}

template <>
void Std::output<ZeroCopyOutput, EndLine>(ZeroCopyOutput& out, EndLine) {
    out.write(u8"\n", 1);
}
