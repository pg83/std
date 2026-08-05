import os
import build


build.cxxflags += [
    "-W",
    "-Wall",
    "-std=c++26",
    "-O2",
    "-g",
    "-fno-omit-frame-pointer",
    "-mno-omit-leaf-frame-pointer",
]
if build.target.startswith("x86_64"):
    build.cxxflags += ["-mcx16"]
std_sources = sorted(build.glob("$(S)/std/*/*.cpp"))
unit_sources = [source for source in std_sources if source.endswith("_ut.cpp")]
library_sources = [source for source in std_sources if not source.endswith("_ut.cpp")]

external_monotonic_clock = "-DSTL_EXTERNAL_MONOTONIC_NOW_US=1" in build.cppflags
libstd_name = "libstd_external_clock" if external_monotonic_clock else "libstd"

libstd = library(
    name=libstd_name,
    srcs=library_sources,
    output=f"$(B)/{libstd_name}.a",
)

test_sources = sorted(build.glob("$(S)/tst/*.cpp"))
test_main = "$(S)/tst/test.cpp"
test = program(
    srcs=[test_main, *unit_sources],
    output="$(B)/tst/test",
    deps=[libstd],
)

test_binaries = []
for source in test_sources:
    if source == test_main:
        continue
    name = os.path.basename(source).removesuffix(".cpp")
    test_binaries.append(program(
        name=name,
        srcs=[source],
        output=f"$(B)/tst/{name}",
        deps=[libstd],
    ))


install(libstd, test, *test_binaries)
