#include "num_cpu.h"

#include <std/str/view.h>
#include <std/lib/buffer.h>
#include <std/ios/fs_utils.h>

#include <stdlib.h>

#if defined(__linux__)
    #include <sched.h>
#elif defined(__APPLE__) || defined(__FreeBSD__)
    #include <sys/sysctl.h>
#endif

using namespace stl;

namespace {
    static u32 fromEnv() noexcept {
        if (auto* v = getenv("GOMAXPROCS")) {
            auto n = StringView(v).stou();

            if (n > 0) {
                return (u32)n;
            }
        }

        return 0;
    }

#if defined(__linux__)
    static u32 readCgroupVal(const char* path) {
        Buffer p{StringView(path)};
        Buffer buf;

        readFileContent(p, buf);

        return (u32)StringView(buf).stripCr().stou();
    }

    static u32 fromCgroupV2() {
        Buffer p(StringView("/sys/fs/cgroup/cpu.max"));
        Buffer buf;

        readFileContent(p, buf);

        StringView line(buf);
        StringView quotaStr, periodStr;

        if (line.stripCr().split(' ', quotaStr, periodStr) && quotaStr != StringView("max")) {
            auto quota = quotaStr.stou();
            auto period = periodStr.stou();

            if (quota > 0 && period > 0) {
                return (u32)((quota + period - 1) / period);
            }
        }

        return 0;
    }

    static u32 fromCgroupV1() {
        auto quota = readCgroupVal("/sys/fs/cgroup/cpu/cpu.cfs_quota_us");
        auto period = readCgroupVal("/sys/fs/cgroup/cpu/cpu.cfs_period_us");

        if (!period) {
            period = 100000;
        }

        if (quota > 0 && period > 0) {
            return (u32)((quota + period - 1) / period);
        }

        return 0;
    }

    static u32 fromOs() noexcept {
        cpu_set_t set;

        if (sched_getaffinity(0, sizeof(set), &set) == 0) {
            return (u32)CPU_COUNT(&set);
        }

        return 1;
    }
#elif defined(__APPLE__) || defined(__FreeBSD__)
    static u32 fromOs() noexcept {
        int n = 0;
        size_t len = sizeof(n);

        if (sysctlbyname("hw.ncpu", &n, &len, nullptr, 0) == 0 && n > 0) {
            return (u32)n;
        }

        return 1;
    }
#else
    static u32 fromOs() noexcept {
        return 1;
    }
#endif
}

u32 stl::numCpu() noexcept {
    if (auto n = fromEnv()) {
        return n;
    }

#if defined(__linux__)
    try {
        if (auto n = fromCgroupV2()) {
            return n;
        }
    } catch (...) {
    }

    try {
        if (auto n = fromCgroupV1()) {
            return n;
        }
    } catch (...) {
    }
#endif

    return fromOs();
}
