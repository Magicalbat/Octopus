
#if defined(PLATFORM_WIN32)
#   include "win32/win32_common.h"
#elif defined(PLATFORM_LINUX)
#endif

#if defined(WIN_GFX_API_OPENGL)
#   include "opengl/opengl_api.h"
#   if defined(PLATFORM_WIN32)
#       include "win32/win32_opengl.h"
#   elif defined(PLATFORM_LINUX)
#   endif
#endif

typedef enum {
    WIN_FLAG_NONE         = 0b0,
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
    WIN_KEY_SHIFT,
    WIN_KEY_CONTROL,
    WIN_KEY_ALT,
    WIN_KEY_0 = '0',
    WIN_KEY_1 = '1',
    WIN_KEY_2 = '2',
    WIN_KEY_3 = '3',
    WIN_KEY_4 = '4',
    WIN_KEY_5 = '5',
    WIN_KEY_6 = '6',
    WIN_KEY_7 = '7',
    WIN_KEY_8 = '8',
    WIN_KEY_9 = '9',
    WIN_KEY_SEMICOLON,
    WIN_KEY_EQUAL,
    WIN_KEY_A = 'A',
    WIN_KEY_B = 'B',
    WIN_KEY_C = 'C',
    WIN_KEY_D = 'D',
    WIN_KEY_E = 'E',
    WIN_KEY_F = 'F',
    WIN_KEY_G = 'G',
    WIN_KEY_H = 'H',
    WIN_KEY_I = 'I',
    WIN_KEY_J = 'J',
    WIN_KEY_K = 'K',
    WIN_KEY_L = 'L',
    WIN_KEY_M = 'M',
    WIN_KEY_N = 'N',
    WIN_KEY_O = 'O',
    WIN_KEY_P = 'P',
    WIN_KEY_Q = 'Q',
    WIN_KEY_R = 'R',
    WIN_KEY_S = 'S',
    WIN_KEY_T = 'T',
    WIN_KEY_U = 'U',
    WIN_KEY_V = 'V',
    WIN_KEY_W = 'W',
    WIN_KEY_X = 'X',
    WIN_KEY_Y = 'Y',
    WIN_KEY_Z = 'Z',
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
    struct _win_plat_info* plat_info;
    struct _win_gfx_info* gfx_info;

    v4_f32 clear_color;

    u32 width, height;

    u32 flags;

    // Physical DPI of the monitor
    u32 raw_dpi;
    // DPI given by the OS for scaling UI
    u32 dpi;

    // Increments of scroll deltas
    // i.e. +1 indicates the wheel was rotated one increment up
    // Floating point to allow finer control for more dynamic input devices
    v2_f32 mouse_scroll;
    f32 touchpad_zoom;

    v2_f32 mouse_pos;

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

void win_gfx_backend_init(void);

window* win_create(mem_arena* arena, u32 width, u32 height, string8 title);
void win_destroy(window* win);

void win_make_current(window* win);

void win_process_events(window* win);

void win_begin_frame(window* win);
void win_end_frame(window* win);

