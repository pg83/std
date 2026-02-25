#include "fs.h"

#include <std/alg/defer.h>
#include <std/sys/throw.h>
#include <std/lib/buffer.h>
#include <std/str/builder.h>

#include <dirent.h>
#include <sys/types.h>

using namespace Std;

void Std::listDirImpl(StringView path, VisitorFace&& vis) {
    DIR* dir = opendir(Buffer(path).cStr());

    if (!dir) {
        Errno().raise(StringBuilder() << StringView(u8"opendir() failed for ") << path);
    }

    STD_DEFER {
        if (closedir(dir) != 0) {
            Errno().raise(StringBuilder() << StringView(u8"closedir() failed for ") << path);
        }
    };

    while (struct dirent* entry = readdir(dir)) {
        StringView name((const char*)entry->d_name);

        if (name == StringView(u8".") || name == StringView(u8"..")) {
            continue;
        }

        TPathInfo pi = {
            .item = name,
            .isDir = entry->d_type == DT_DIR,
        };

        vis.visit(&pi);
    }
}
