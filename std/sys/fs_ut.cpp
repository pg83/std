#include "fs.h"

#include <std/tst/ut.h>
#include <std/lib/vector.h>

using namespace stl;

STD_TEST_SUITE(ListDir) {
    STD_TEST(CurrentDirectoryBasic) {
        Vector<TPathInfo> items;

        listDir(StringView(u8"."), [&items](const TPathInfo& pi) {
            items.pushBack(pi);
        });

        u64 dirCount = 0;
        u64 fileCount = 0;

        for (u64 i = 0; i < items.length(); ++i) {
            if (items[i].isDir) {
                dirCount++;
            } else {
                fileCount++;
            }
        }

        STD_INSIST(dirCount > 2);
        STD_INSIST(fileCount > 2);
    }

    STD_TEST(CurrentDirectoryContainsExpectedItems) {
        bool foundTstDir = false;
        bool foundStdDir = false;
        bool foundMakefile = false;

        listDir(StringView(u8"."), [&](const TPathInfo& pi) {
            if (pi.item == StringView(u8"tst") && pi.isDir) {
                foundTstDir = true;
            }
            if (pi.item == StringView(u8"std") && pi.isDir) {
                foundStdDir = true;
            }
            if (pi.item == StringView(u8"Makefile") && !pi.isDir) {
                foundMakefile = true;
            }
        });

        STD_INSIST(foundTstDir);
        STD_INSIST(foundStdDir);
        STD_INSIST(foundMakefile);
    }
}
