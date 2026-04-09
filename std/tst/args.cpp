#include "args.h"

#include <std/str/view.h>

using namespace stl;

TestArgs::TestArgs(ObjPool* pool, int argc, char** argv)
    : HashMap(pool)
{
    for (int i = 1; i < argc; ++i) {
        StringView arg(argv[i]);

        if (arg.startsWith(u8"--") && arg.length() > 2) {
            StringView key(arg.data() + 2, arg.length() - 2);
            StringView value(u8"1");

            if (auto eq = key.memChr('='); eq) {
                value = StringView(eq + 1, key.end());
                key = StringView(key.begin(), eq);
            }

            (*this)[key] = value;
        }
    }
}
