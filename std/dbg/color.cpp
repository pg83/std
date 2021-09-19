#include "color.h"

#include <std/ios/buf.h>

using namespace Std;

template <>
void Std::output<OutBuf, Color>(OutBuf& buf, Color color) {
    buf << StringView(u8"\033[");

    if (color.color == AnsiColor::Reset) {
        buf << StringView(u8"0");
    } else {
        auto add = color.brightKind ? 60 : 0;

        buf << (u64)(29 + add + (int)color.color);
    }

    buf << StringView(u8"m");
}
