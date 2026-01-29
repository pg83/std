#!/usr/bin/env sh

set -xue

ya style std tst

(
    find . -type f -name '*.cpp'
    find . -type f -name '*.h'
) | while read l; do
    sed -e 's| // namespace.*||' -i ${l}
done
