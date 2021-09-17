#include <std/tl/range.h>
#include <std/tl/buffer.h>
#include <std/io/output.h>
#include <std/tl/string/dynamic.h>

//#include <string>

using namespace Std;

int main() {
#if 0
    DynString buf = u8"test";

    buf.append(u8"qw", 2);
    buf.append(u8"er", 2);

    for (size_t i = 0; i < 10; ++i) {
        buf += 'x';
    }

    for (auto ch : range(buf)) {
        stdO << ch << endL;
    }
#endif

    for (size_t i = 0; i < 1000000; ++i) {
        DynString s;
        //std::string s;

        for (size_t j = 0; j < 1000; ++j) {
            s += 'x';
        }
    }
}
