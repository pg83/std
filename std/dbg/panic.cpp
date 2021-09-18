#include "panic.h"

#include <std/str/view.h>
#include <std/ios/output.h>

#include <stdlib.h>

void Std::panic(const u8* what, u32 line, const u8* file) {
    stdE << StringView(u8"assertion failed: ") << what
         << StringView(u8", at ") << file
         << StringView(u8":") << line
         << endL;

    abort();
}
