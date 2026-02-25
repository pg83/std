#pragma once

#include <std/str/view.h>

namespace Std {
    struct TPathInfo {
        StringView item;
        bool isDir;
    };

    struct TFsVisitor {
        virtual void visit(TPathInfo pi) = 0;
    };

    void listDir(TFsVisitor&& vis);
}
