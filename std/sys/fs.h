#pragma once

#include <std/str/view.h>
#include <std/lib/visitor.h>

namespace Std {
    struct TPathInfo {
        StringView item;
        bool isDir;
    };

    void listDir(StringView path, VisitorFace&& vis);

    template <typename F>
    inline void listDir(StringView path, F f) {
        listDir(makeVisitor([f](void* ptr) {
            f(*(TPathInfo*)ptr);
        }));
    }
}
