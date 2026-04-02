Build and test: `sh ./run.sh`.
CamelCase for test names in STD_TEST.
Git author: `claude <claude@users.noreply.github.com>`. Commit messages in English.
Free classes and functions in .cpp go into anonymous namespace.
Methods longer than 1 line must be defined out of line and outside any namespace.
Each xxx.h must have a corresponding xxx.cpp.
No single-line braced blocks: `{ ... }` must span multiple lines.
Avoid includes in headers; prefer forward declarations. Only include in .cpp files.
