#define WIN_API_OPENGL

#if defined(WIN_API_OPENGL)

#include "opengl/opengl_defs.h"
#include "opengl/opengl_helpers.h"

#endif

#define WIN_PEN_CACHE_SIZE 64

typedef enum {
    WIN_PEN_FLAG_NONE      = 0b00000001,
    WIN_PEN_FLAG_ERASER    = 0b00000010,
    WIN_PEN_FLAG_HOVERING  = 0b00000100,
    WIN_PEN_FLAG_DOWN      = 0b00001000,
    WIN_PEN_FLAG_JUST_DOWN = 0b00010000,
    WIN_PEN_FLAG_JUST_UP   = 0b00100000,
} win_pen_flags;

typedef struct {
    vec2f pos;
    // From 0 to 1
    // If the pen does not support pressure,
    // pressure will be 1 by default
    f32 pressure;
    // From 0 to 2pi
    f32 rotation;
    // From -pi/2 to +pi/2
    vec2f tilt;

    // See win_pen_flags enum
    u32 flags;
} win_pen_sample;

typedef enum {
    WIN_MB_LEFT,
    WIN_MB_MIDDLE,
    WIN_MB_RIGHT, 
    WIN_MB_4,
    WIN_MB_5,

    WIN_MB_COUNT
} win_mouse_button;

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
    WIN_KEY_LEFT,
    WIN_KEY_UP,
    WIN_KEY_RIGHT,
    WIN_KEY_DOWN,
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
} win_key;

typedef struct {
    struct _win_backend* backend;

    string8 title;
    u32 width, height;

    b32 should_close;

    win_pen_sample last_pen_sample;
    // If num_pen_samples is 0,
    // you can assume the pen is not hovering and not in contact
    u32 num_pen_samples;
    // pen_samples[0] is the oldest sample, and 
    // pen_samples[num_pen_samples-1] is the newest
    win_pen_sample pen_samples[WIN_PEN_CACHE_SIZE];

    i32 mouse_scroll;
    vec2f mouse_pos;
    b8 mouse_buttons[WIN_MB_COUNT];
    b8 prev_mouse_buttons[WIN_MB_COUNT];

    b8 keys[WIN_KEY_COUNT];
    b8 prev_keys[WIN_KEY_COUNT];
} win_window;

win_window* window_create(mem_arena* arena, u32 width, u32 height, string8 title);
void window_destroy(win_window* win);

b32 window_make_current(win_window* win);

void window_process_events(win_window* win);

void window_clear_color(win_window* win, vec4f col);
void window_clear(win_window* win);

void window_swap_buffers(win_window* win);

#define WIN_MOUSE_DOWN(win, mb) (win->mouse_buttons[mb])
#define WIN_MOUSE_UP(win, mb) (!win->mouse_buttons[mb])
#define WIN_MOUSE_JUST_DOWN(win, mb) (win->mouse_buttons[mb] && !win->prev_mouse_buttons[mb])
#define WIN_MOUSE_JUST_UP(win, mb) (!win->mouse_buttons[mb] && win->prev_mouse_buttons[mb])

#define WIN_KEY_DOWN(win, key) (win->keys[key])
#define WIN_KEY_UP(win, key) (!win->keys[key])
#define WIN_KEY_JUST_DOWN(win, key) (win->keys[key] && !win->prev_keys[key])
#define WIN_KEY_JUST_UP(win, key) (!win->keys[key] && win->prev_keys[key])

