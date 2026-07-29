import os
import platform

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
if platform.machine() == "x86_64":
    build.cxxflags += ["-Werror"]


std_sources = sorted(build.glob("$(S)/std/*/*.cpp"))
unit_sources = [source for source in std_sources if source.endswith("_ut.cpp")]
library_sources = [source for source in std_sources if not source.endswith("_ut.cpp")]

libstd = library(
    srcs=library_sources,
    output="$(B)/libstd.a",
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
