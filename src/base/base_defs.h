#if defined(_WIN32)
#   define PLATFORM_WIN32
#elif defined(__EMSCRIPTEN__)
#   define PLATFORM_WASM
#elif defined(__linux__)
#   define PLATFORM_LINUX
#endif

#if defined(__clang__) || defined(__GNUC__)
#    define THREAD_LOCAL __thread
#elif defined(_MSC_VER)
#    define THREAD_LOCAL __declspec(thread)
#elif (__STDC_VERSION__ >= 201112L)
#    define THREAD_LOCAL _Thread_local
#else
#    error "Invalid compiler/version for thead variable; Use Clang, GCC, or MSVC, or use C11 or greater"
#endif

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

#define STRINGIFY_NX(s) #s
#define STRINGIFY(s) STRINGIFY_NX(s)

#define KiB(n) ((u64)(n) << 10)
#define MiB(n) ((u64)(n) << 20)
#define GiB(n) ((u64)(n) << 30)

#define ALIGN_UP_POW2(n, p) (((u64)(n) + ((u64)(p) - 1)) & (~((u64)(p) - 1)))
#define ALIGN_DOWN_POW2(n, p) (((u64)(n)) & (~((u64)(p) - 1)))

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

