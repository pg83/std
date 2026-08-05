#pragma once

// portable defines for compiler intrinsics

#define stdIsTriviallyCopyable __is_trivially_copyable
#define stdHasTrivialDestructor __is_trivially_destructible
#define stdIsTrivial __is_trivial
#define stdIsStandardLayout __is_standard_layout
