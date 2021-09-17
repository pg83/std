#include <std/tl/buffer.h>
#include <std/io/output.h>

using namespace Std;

int main() {
    Buffer buf(1024);

    buf.append("qw", 2);

    stdOut << buf;
}
