
#if defined(PLATFORM_WIN32)
#   include "win32/win32_common.h"
#elif defined(PLATFORM_LINUX)
#endif

#if defined(WIN_GFX_API_OPENGL)
#   include "opengl/opengl_common.h"
#   if defined(PLATFORM_WIN32)
#       include "win32/win32_opengl.h"
#   elif defined(PLATFORM_LINUX)
#   endif
#endif

typedef enum {
    WIN_FLAG_NONE = 0b0,
    WIN_FLAG_SHOULD_CLOSE = 0b1,
} win_flags;

typedef u8 win_mouse_button;
typedef u8 win_key;

typedef enum {
    WIN_MB_LEFT,
    WIN_MB_MIDDLE,
    WIN_MB_RIGHT, 
    WIN_MB_4,
    WIN_MB_5,

    WIN_MB_COUNT
} _win_mouse_button_enum;

STATIC_ASSERT(
    WIN_MB_COUNT < (1 << (sizeof(win_mouse_button) * 8)),
    num_mouse_buttons
);

typedef enum {
    WIN_KEY_NONE = 0,
    WIN_KEY_BACKSPACE,
    WIN_KEY_TAB,
    WIN_KEY_ENTER,
    WIN_KEY_CAPSLOCK,
    WIN_KEY_ESCAPE,
    WIN_KEY_SPACE,
    WIN_KEY_PAGEUP,
    WIN_KEY_PAGEDOWN,
    WIN_KEY_END,
    WIN_KEY_HOME,
    WIN_KEY_ARROW_LEFT,
    WIN_KEY_ARROW_UP,
    WIN_KEY_ARROW_RIGHT,
    WIN_KEY_ARROW_DOWN,
    WIN_KEY_INSERT,
    WIN_KEY_DELETE,
    WIN_KEY_NUMPAD_0,
    WIN_KEY_NUMPAD_1,
    WIN_KEY_NUMPAD_2,
    WIN_KEY_NUMPAD_3,
    WIN_KEY_NUMPAD_4,
    WIN_KEY_NUMPAD_5,
    WIN_KEY_NUMPAD_6,
    WIN_KEY_NUMPAD_7,
    WIN_KEY_NUMPAD_8,
    WIN_KEY_NUMPAD_9,
    WIN_KEY_NUMPAD_MULTIPLY,
    WIN_KEY_NUMPAD_ADD,
    WIN_KEY_NUMPAD_SUBTRACT,
    WIN_KEY_NUMPAD_DECIMAL,
    WIN_KEY_NUMPAD_DIVIDE,
    WIN_KEY_NUMPAD_ENTER,
    WIN_KEY_F1,
    WIN_KEY_F2,
    WIN_KEY_F3,
    WIN_KEY_F4,
    WIN_KEY_F5,
    WIN_KEY_F6,
    WIN_KEY_F7,
    WIN_KEY_F8,
    WIN_KEY_F9,
    WIN_KEY_F10,
    WIN_KEY_F11,
    WIN_KEY_F12,
    WIN_KEY_NUM_LOCK,
    WIN_KEY_SCROLL_LOCK,
    WIN_KEY_LSHIFT,
    WIN_KEY_0,
    WIN_KEY_1,
    WIN_KEY_2,
    WIN_KEY_3,
    WIN_KEY_4,
    WIN_KEY_5,
    WIN_KEY_6,
    WIN_KEY_7,
    WIN_KEY_8,
    WIN_KEY_9,
    WIN_KEY_RSHIFT,
    WIN_KEY_LCONTROL,
    WIN_KEY_RCONTROL,
    WIN_KEY_LALT,
    WIN_KEY_RALT,
    WIN_KEY_SEMICOLON,
    WIN_KEY_EQUAL,
    WIN_KEY_A,
    WIN_KEY_B,
    WIN_KEY_C,
    WIN_KEY_D,
    WIN_KEY_E,
    WIN_KEY_F,
    WIN_KEY_G,
    WIN_KEY_H,
    WIN_KEY_I,
    WIN_KEY_J,
    WIN_KEY_K,
    WIN_KEY_L,
    WIN_KEY_M,
    WIN_KEY_N,
    WIN_KEY_O,
    WIN_KEY_P,
    WIN_KEY_Q,
    WIN_KEY_R,
    WIN_KEY_S,
    WIN_KEY_T,
    WIN_KEY_U,
    WIN_KEY_V,
    WIN_KEY_W,
    WIN_KEY_X,
    WIN_KEY_Y,
    WIN_KEY_Z,
    WIN_KEY_COMMA,
    WIN_KEY_PERIOD,
    WIN_KEY_MINUS,
    WIN_KEY_FORWARDSLASH,
    WIN_KEY_BACKSLASH,
    WIN_KEY_BACKTICK,
    WIN_KEY_LBRACKET,
    WIN_KEY_RBRACKET,
    WIN_KEY_APOSTROPHE,

    WIN_KEY_COUNT
} _win_key_enum;

STATIC_ASSERT(
    WIN_KEY_COUNT < (1 << (sizeof(win_key) * 8)),
    num_keys
);

typedef struct {
    struct _win_plat_backend* plat_backend;
    struct _win_gfx_backend* gfx_backend;

    u32 width, height;

    u32 flags;

    // Increments of scroll deltas
    // i.e. +1 indicates the wheel was rotated one increment up
    // Floating point to allow finer control for more dynamic input devices
    vec2f mouse_scroll;
    f32 touchpad_zoom;

    vec2f mouse_pos;

    b8 mouse_buttons[WIN_MB_COUNT];
    b8 prev_mouse_buttons[WIN_MB_COUNT];

    b8 keys[WIN_KEY_COUNT];
    b8 prev_keys[WIN_KEY_COUNT];
} window;

#define WIN_MOUSE_DOWN(win, mb) (win->mouse_buttons[mb])
#define WIN_MOUSE_UP(win, mb) (!win->mouse_buttons[mb])
#define WIN_MOUSE_JUST_DOWN(win, mb) (win->mouse_buttons[mb] && !win->prev_mouse_buttons[mb])
#define WIN_MOUSE_JUST_UP(win, mb) (!win->mouse_buttons[mb] && win->prev_mouse_buttons[mb])

#define WIN_KEY_DOWN(win, key) (win->keys[key])
#define WIN_KEY_UP(win, key) (!win->keys[key])
#define WIN_KEY_JUST_DOWN(win, key) (win->keys[key] && !win->prev_keys[key])
#define WIN_KEY_JUST_UP(win, key) (!win->keys[key] && win->prev_keys[key])

b32 win_gfx_backend_init(void);

window* win_create(mem_arena* arena, u32 width, u32 height, string8 title);
void win_destroy(window* win);

void win_make_current(window* win);

void win_process_events(window* win);

void win_set_clear_color(window* win, vec4f col);

void win_begin_frame(window* win);
void win_end_frame(window* win);

