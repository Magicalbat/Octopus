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

#define GFX_KEY_XLIST \
    X(NONE, = 0) \
    X(BACKSPACE, ) \
    X(TAB, ) \
    X(ENTER, ) \
    X(CAPSLOCK, ) \
    X(ESCAPE, ) \
    X(SPACE, ) \
    X(PAGEUP, ) \
    X(PAGEDOWN, ) \
    X(END, ) \
    X(HOME, ) \
    X(LEFT, ) \
    X(UP, ) \
    X(RIGHT, ) \
    X(DOWN, ) \
    X(INSERT, ) \
    X(DELETE, ) \
    X(0, = '0') \
    X(1, = '1') \
    X(2, = '2') \
    X(3, = '3') \
    X(4, = '4') \
    X(5, = '5') \
    X(6, = '6') \
    X(7, = '7') \
    X(8, = '8') \
    X(9, = '9') \
    X(A, = 'A') \
    X(B, = 'B') \
    X(C, = 'C') \
    X(D, = 'D') \
    X(E, = 'E') \
    X(F, = 'F') \
    X(G, = 'G') \
    X(H, = 'H') \
    X(I, = 'I') \
    X(J, = 'J') \
    X(K, = 'K') \
    X(L, = 'L') \
    X(M, = 'M') \
    X(N, = 'N') \
    X(O, = 'O') \
    X(P, = 'P') \
    X(Q, = 'Q') \
    X(R, = 'R') \
    X(S, = 'S') \
    X(T, = 'T') \
    X(U, = 'U') \
    X(V, = 'V') \
    X(W, = 'W') \
    X(X, = 'X') \
    X(Y, = 'Y') \
    X(Z, = 'Z') \
    X(NUMPAD_0, ) \
    X(NUMPAD_1, ) \
    X(NUMPAD_2, ) \
    X(NUMPAD_3, ) \
    X(NUMPAD_4, ) \
    X(NUMPAD_5, ) \
    X(NUMPAD_6, ) \
    X(NUMPAD_7, ) \
    X(NUMPAD_8, ) \
    X(NUMPAD_9, ) \
    X(NUMPAD_MULTIPLY, ) \
    X(NUMPAD_ADD, ) \
    X(NUMPAD_SUBTRACT, ) \
    X(NUMPAD_DECIMAL, ) \
    X(NUMPAD_DIVIDE, ) \
    X(NUMPAD_ENTER, ) \
    X(F1, ) \
    X(F2, ) \
    X(F3, ) \
    X(F4, ) \
    X(F5, ) \
    X(F6, ) \
    X(F7, ) \
    X(F8, ) \
    X(F9, ) \
    X(F10, ) \
    X(F11, ) \
    X(F12, ) \
    X(NUM_LOCK, ) \
    X(SCROLL_LOCK, ) \
    X(LSHIFT, ) \
    X(RSHIFT, ) \
    X(LCONTROL, ) \
    X(RCONTROL, ) \
    X(LALT, ) \
    X(RALT, ) \
    X(SEMICOLON, ) \
    X(EQUAL, ) \
    X(COMMA, ) \
    X(PERIOD, ) \
    X(MINUS, ) \
    X(FORWARDSLASH, ) \
    X(BACKSLASH, ) \
    X(BACKTICK, ) \
    X(LBRACKET, ) \
    X(RBRACKET, ) \
    X(APOSTROPHE, )

typedef enum {
#define X(key, value) GFX_KEY_##key value,
    GFX_KEY_XLIST
#undef X

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

#define GFX_IS_MOUSE_DOWN(win, mb) ( win->mouse_buttons[mb])
#define GFX_IS_MOUSE_UP(win, mb)   (!win->mouse_buttons[mb])
#define GFX_IS_MOUSE_JUST_DOWN(win, mb) (win->mouse_buttons[mb] && !win->prev_mouse_buttons[mb])
#define GFX_IS_MOUSE_JUST_UP(win, mb) (!win->mouse_buttons[mb] && win->prev_mouse_buttons[mb])

#define GFX_IS_KEY_DOWN(win, key) (win->keys[key])
#define GFX_IS_KEY_UP(win, key) (!win->keys[key])
#define GFX_IS_KEY_JUST_DOWN(win, key) (win->keys[key] && !win->prev_keys[key])
#define GFX_IS_KEY_JUST_UP(win, key) (!win->keys[key] && win->prev_keys[key])


#endif // GFX_H

