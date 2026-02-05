#include "throw.h"

#include <std/tst/ut.h>
#include <std/str/view.h>
#include <std/lib/buffer.h>
#include <std/str/builder.h>
#include <std/typ/support.h>

#include <errno.h>
#include <string.h>

using namespace Std;

namespace {
    static inline bool contains(StringView haystack, StringView needle) {
        return haystack.search(needle) != (size_t)-1;
    }
}

STD_TEST_SUITE(ThrowErrno) {
    STD_TEST(BasicThrow) {
        try {
            throwErrno(EINVAL, StringBuilder() << StringView(u8"test error"));
            STD_INSIST(false);
        } catch (Exception& e) {
            STD_INSIST(e.kind() == ExceptionKind::Errno);
            StringView desc = e.description();
            STD_INSIST(contains(desc, StringView(u8"test error")));
            STD_INSIST(contains(desc, StringView(u8"code")));

            return;
        }

        STD_INSIST(false);
    }

    STD_TEST(ErrorCodeInDescription) {
        try {
            throwErrno(ENOENT, StringBuilder() << StringView(u8"file not found"));
            STD_INSIST(false);
        } catch (Exception& e) {
            StringView desc = e.description();
            STD_INSIST(contains(desc, StringView(u8"file not found")));
            STD_INSIST(contains(desc, StringView(u8"code")));
            STD_INSIST(contains(desc, StringView(u8"descr")));
        }
    }

    STD_TEST(CustomMessage) {
        try {
            throwErrno(EACCES, StringBuilder() << StringView(u8"custom error message"));
            STD_INSIST(false);
        } catch (Exception& e) {
            StringView desc = e.description();
            STD_INSIST(contains(desc, StringView(u8"custom error message")));
        }
    }

    STD_TEST(EmptyMessage) {
        try {
            throwErrno(EPERM, StringBuilder());
            STD_INSIST(false);
        } catch (Exception& e) {
            STD_INSIST(e.kind() == ExceptionKind::Errno);
            StringView desc = e.description();
            STD_INSIST(contains(desc, StringView(u8"code")));
        }
    }

    STD_TEST(MultipleDescriptionCalls) {
        try {
            throwErrno(EIO, StringBuilder() << StringView(u8"test message"));
            STD_INSIST(false);
        } catch (Exception& e) {
            StringView desc1 = e.description();
            StringView desc2 = e.description();
            STD_INSIST(contains(desc1, StringView(u8"test message")));
            STD_INSIST(contains(desc2, StringView(u8"test message")));
        }
    }

    STD_TEST(StringBuilderConstruction) {
        try {
            throwErrno(ENOENT, StringBuilder()
                                   << StringView(u8"error opening file: ")
                                   << StringView(u8"/path/to/file"));
            STD_INSIST(false);
        } catch (Exception& e) {
            StringView desc = e.description();
            STD_INSIST(contains(desc, StringView(u8"error opening file: ")));
            STD_INSIST(contains(desc, StringView(u8"/path/to/file")));
        }
    }

    STD_TEST(ZeroErrorCode) {
        try {
            throwErrno(0, StringBuilder() << StringView(u8"zero error"));
            STD_INSIST(false);
        } catch (Exception& e) {
            STD_INSIST(e.kind() == ExceptionKind::Errno);
            StringView desc = e.description();
            STD_INSIST(contains(desc, StringView(u8"zero error")));
            STD_INSIST(contains(desc, StringView(u8"code 0")));
        }
    }

    STD_TEST(LongMessage) {
        try {
            StringBuilder sb;
            for (int i = 0; i < 100; i++) {
                sb << StringView(u8"word ");
            }
            throwErrno(ENOMEM, move(sb));
            STD_INSIST(false);
        } catch (Exception& e) {
            StringView desc = e.description();
            STD_INSIST(contains(desc, StringView(u8"word word word")));
        }
    }

    STD_TEST(MessageWithNumbers) {
        try {
            throwErrno(EINVAL, StringBuilder()
                                   << StringView(u8"error at line ")
                                   << 42
                                   << StringView(u8" column ")
                                   << 15);
            STD_INSIST(false);
        } catch (Exception& e) {
            StringView desc = e.description();
            STD_INSIST(contains(desc, StringView(u8"error at line")));
            STD_INSIST(contains(desc, StringView(u8"42")));
            STD_INSIST(contains(desc, StringView(u8"column")));
            STD_INSIST(contains(desc, StringView(u8"15")));
        }
    }

    STD_TEST(SystemErrorMessage) {
        try {
            throwErrno(EINVAL, StringBuilder() << StringView(u8"invalid argument"));
            STD_INSIST(false);
        } catch (Exception& e) {
            StringView desc = e.description();
            STD_INSIST(contains(desc, StringView(u8"invalid argument")));
            const char* sysMsg = strerror(EINVAL);
            if (sysMsg && strlen(sysMsg) > 0) {
                STD_INSIST(contains(desc, StringView(sysMsg)));
            }
        }
    }
}
