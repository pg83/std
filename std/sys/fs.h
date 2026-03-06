#pragma once

#include <std/str/view.h>
#include <std/lib/visitor.h>

namespace stl {
    struct TPathInfo {
        StringView item;
        bool isDir;
    };

    void listDirImpl(StringView path, VisitorFace&& vis);

    template <typename F>
    inline void listDir(StringView path, F f) {
        listDirImpl(path, makeVisitor([f](void* ptr) {
                        f(*(TPathInfo*)ptr);
                    }));
    }
}
