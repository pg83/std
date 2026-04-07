#!/usr/bin/env sh

(
    find . -type f -name '*.h'
    find . -type f -name '*.cpp'
) | grep -v '_ut.cpp' | while read l; do
    echo ${l}
    cat ${l}
done
