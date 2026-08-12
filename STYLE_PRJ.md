# Project style settings

Per-project settings that the shared [STYLE.md](STYLE.md) delegates here.

- **Macro prefix.** Project-owned macros use `STD_` (`STD_ASSERT`,
  `STD_INSIST`, `STD_VERIFY`, `STD_TEST`, ...).
- **Namespace.** This project is the library itself: public API lives in the
  `stl` namespace.
- **Formatter.** `./style.py` formats every tracked C++ source.

## Deviations

- The vocabulary rules in the shared style (`stl::` containers, no `std::`)
  describe what this library provides; internally it implements them on top
  of the C library and the language runtime.
