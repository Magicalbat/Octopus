#ifndef GFX_H
#define GFX_H

#include "base/base.h"

typedef enum {
    GFX_MB_LEFT,
    GFX_MB_MIDDLE,
    GFX_MB_RIGHT, 
    GFX_MB_4,
    GFX_MB_5,

    GFX_MB_COUNT
} gfx_mouse_button;

typedef enum {
    GFX_KEY_NONE = 0,
    GFX_KEY_BACKSPACE,
    GFX_KEY_TAB,
    GFX_KEY_ENTER,
    GFX_KEY_CAPSLOCK,
    GFX_KEY_ESCAPE,
    GFX_KEY_SPACE,
    GFX_KEY_PAGEUP,
    GFX_KEY_PAGEDOWN,
    GFX_KEY_END,
    GFX_KEY_HOME,
    GFX_KEY_LEFT,
    GFX_KEY_UP,
    GFX_KEY_RIGHT,
    GFX_KEY_DOWN,
    GFX_KEY_INSERT,
    GFX_KEY_DELETE,
    GFX_KEY_NUMPAD_0,
    GFX_KEY_NUMPAD_1,
    GFX_KEY_NUMPAD_2,
    GFX_KEY_NUMPAD_3,
    GFX_KEY_NUMPAD_4,
    GFX_KEY_NUMPAD_5,
    GFX_KEY_NUMPAD_6,
    GFX_KEY_NUMPAD_7,
    GFX_KEY_NUMPAD_8,
    GFX_KEY_NUMPAD_9,
    GFX_KEY_NUMPAD_MULTIPLY,
    GFX_KEY_NUMPAD_ADD,
    GFX_KEY_NUMPAD_SUBTRACT,
    GFX_KEY_NUMPAD_DECIMAL,
    GFX_KEY_NUMPAD_DIVIDE,
    GFX_KEY_NUMPAD_ENTER,
    GFX_KEY_F1,
    GFX_KEY_F2,
    GFX_KEY_F3,
    GFX_KEY_F4,
    GFX_KEY_F5,
    GFX_KEY_F6,
    GFX_KEY_F7,
    GFX_KEY_F8,
    GFX_KEY_F9,
    GFX_KEY_F10,
    GFX_KEY_F11,
    GFX_KEY_F12,
    GFX_KEY_NUM_LOCK,
    GFX_KEY_SCROLL_LOCK,
    GFX_KEY_LSHIFT,
    GFX_KEY_0,
    GFX_KEY_1,
    GFX_KEY_2,
    GFX_KEY_3,
    GFX_KEY_4,
    GFX_KEY_5,
    GFX_KEY_6,
    GFX_KEY_7,
    GFX_KEY_8,
    GFX_KEY_9,
    GFX_KEY_RSHIFT,
    GFX_KEY_LCONTROL,
    GFX_KEY_RCONTROL,
    GFX_KEY_LALT,
    GFX_KEY_RALT,
    GFX_KEY_SEMICOLON,
    GFX_KEY_EQUAL,
    GFX_KEY_A,
    GFX_KEY_B,
    GFX_KEY_C,
    GFX_KEY_D,
    GFX_KEY_E,
    GFX_KEY_F,
    GFX_KEY_G,
    GFX_KEY_H,
    GFX_KEY_I,
    GFX_KEY_J,
    GFX_KEY_K,
    GFX_KEY_L,
    GFX_KEY_M,
    GFX_KEY_N,
    GFX_KEY_O,
    GFX_KEY_P,
    GFX_KEY_Q,
    GFX_KEY_R,
    GFX_KEY_S,
    GFX_KEY_T,
    GFX_KEY_U,
    GFX_KEY_V,
    GFX_KEY_W,
    GFX_KEY_X,
    GFX_KEY_Y,
    GFX_KEY_Z,
    GFX_KEY_COMMA,
    GFX_KEY_PERIOD,
    GFX_KEY_MINUS,
    GFX_KEY_FORWARDSLASH,
    GFX_KEY_BACKSLASH,
    GFX_KEY_BACKTICK,
    GFX_KEY_LBRACKET,
    GFX_KEY_RBRACKET,
    GFX_KEY_APOSTROPHE,

    GFX_KEY_COUNT
} gfx_key;

typedef struct {
    string8 title;
    u32 width, height;

    b32 should_close;

    vec2f mouse_pos;
    i32 mouse_scroll;
    b8 mouse_buttons[GFX_MB_COUNT];
    b8 prev_mouse_buttons[GFX_MB_COUNT];

    b8 keys[GFX_KEY_COUNT];
    b8 prev_keys[GFX_KEY_COUNT];

    struct _gfx_win_backend* backend;
} gfx_window;

gfx_window* gfx_win_create(mem_arena* arena, u32 width, u32 height, string8 title);
void gfx_win_destroy(gfx_window* win);

b32 gfx_win_make_current(gfx_window* win);

void gfx_win_process_events(gfx_window* win);

void gfx_win_clear_color(gfx_window* win, vec4f col);
void gfx_win_clear(gfx_window* win);

void gfx_win_swap_buffers(gfx_window* win);

#define GFX_IS_MOUSE_DOWN(win, mb) (win->mouse_buttons[mb])
#define GFX_IS_MOUSE_UP(win, mb) (!win->mouse_buttons[mb])
#define GFX_IS_MOUSE_JUST_DOWN(win, mb) (win->mouse_buttons[mb] && !win->prev_mouse_buttons[mb])
#define GFX_IS_MOUSE_JUST_UP(win, mb) (!win->mouse_buttons[mb] && win->prev_mouse_buttons[mb])

#define GFX_IS_KEY_DOWN(win, key) (win->keys[key])
#define GFX_IS_KEY_UP(win, key) (!win->keys[key])
#define GFX_IS_KEY_JUST_DOWN(win, key) (win->keys[key] && !win->prev_keys[key])
#define GFX_IS_KEY_JUST_UP(win, key) (!win->keys[key] && win->prev_keys[key])


#endif // GFX_H

