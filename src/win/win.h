
#include "win_event.h"

#if defined(PLATFORM_WIN32)
#   include "win32/win32_common.h"
#elif defined(PLATFORM_LINUX)
#endif

#if defined(WIN_GFX_API_OPENGL)
#   include "opengl/opengl_api.h"
#   include "opengl/opengl_helpers.h"
#   if defined(PLATFORM_WIN32)
#       include "win32/win32_opengl.h"
#   elif defined(PLATFORM_LINUX)
#   endif
#endif

typedef enum {
    WIN_FLAG_NONE         = 0b0,
    WIN_FLAG_SHOULD_CLOSE = 0b1,
} win_flags;

typedef struct window {
    struct _win_plat_info* plat_info;
    struct _win_gfx_info* gfx_info;

    v4_f32 clear_color;

    u32 width, height;

    u32 flags;

    // Physical DPI of the monitor
    u32 raw_dpi;
    // DPI given by the OS for scaling UI
    u32 dpi;

    v2_f32 prev_mouse_pos;
    b8 prev_mouse_buttons[WIN_MB_COUNT];
    b8 prev_keys[WIN_KEY_COUNT];

    f32 cur_trackpad_zoom;
    v2_f32 cur_scroll;
    v2_f32 cur_mouse_pos;
    b8 cur_mouse_buttons[WIN_MB_COUNT];
    b8 cur_keys[WIN_KEY_COUNT];

    win_event* first_event;
    win_event* last_event;
} window;

void win_gfx_backend_init(void);

window* win_create(mem_arena* arena, u32 width, u32 height, string8 title);
void win_destroy(window* win);

void win_make_current(window* win);

void win_process_events(mem_arena* frame_arena, window* win);

void win_begin_frame(window* win);
void win_end_frame(window* win);

