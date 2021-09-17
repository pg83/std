#include <std/tl/range.h>
#include <std/tl/buffer.h>
#include <std/io/output.h>
#include <std/tl/string/dynamic.h>

using namespace Std;

int main() {
    DynString buf;

    buf.append(u8"qw", 2);
    buf.append(u8"er", 2);

    for (auto ch : range(buf)) {
        stdO << ch << endL;
    }
}
