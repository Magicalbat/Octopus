#ifndef BASE_DEFS_H
#define BASE_DEFS_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef i8 b8;
typedef i32 b32;

typedef float f32;
typedef double f64;

static_assert(sizeof(f32) == 4, "Size of float must be 4");
static_assert(sizeof(f64) == 8, "Size of double must be 8");

#if defined(_WIN32)
#   define PLATFORM_WIN32
#elif defined(__EMSCRIPTEN__)
#   define PLATFORM_WASM
#elif defined(__linux__)
#   define PLATFORM_LINUX
#endif

#define UNUSED(x) (void)(x)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(x, a, b) (MIN((b), MAX((x), (a))))
#define ABS(n) ((n) < 0 ? -(n) : (n))
#define SIGN(n) ((n) < 0 ? -1 : 1)

#define SLL_PUSH_FRONT(f, l, n) ((f) == NULL ? \
    ((f) = (l) = (n)) :                        \
    ((n)->next = (f), (f) = (n)))              \

#define SLL_PUSH_BACK(f, l, n) ((f) == NULL ? \
    ((f) = (l) = (n)) :                       \
    ((l)->next = (n), (l) = (n)),             \
    ((n)->next = NULL))                       \

#define SLL_POP_FRONT(f, l) ((f) == (l) ? \
    ((f) = (l) = NULL) :                  \
    ((f) = (f)->next))                    \

#endif // BASE_DEFS_H
