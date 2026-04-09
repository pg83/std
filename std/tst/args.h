#pragma once

#include <std/sym/s_map.h>

namespace stl {
    class ObjPool;

    class TestArgs: public SymbolMap<StringView> {
    public:
        TestArgs(ObjPool* pool)
            : HashMap(pool)
        {
        }

        void parse(StringView arg);

        TestArgs(ObjPool* pool, int argc, char** argv);
    };
}
