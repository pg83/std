#!/usr/bin/env sh

set -xue

find . -type f -name '*_ut.cpp' | while read l; do sed -e 's|{ |{\n|' -i ${l}; done
find . -type f -name '*_ut.cpp' | while read l; do sed -e 's|; }|;\n}|' -i ${l}; done

ya style std tst

(
    find . -type f -name '*.cpp'
    find . -type f -name '*.h'
) | while read l; do
    sed -e 's| // namespace.*||' -i ${l}
done
