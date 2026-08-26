
#define W32_WIN_CLASS_NAME L"OctopusWindow"

#include "win32_trackpad.h"

#include <ShellScalingApi.h>

typedef struct window window;

typedef struct {
    mem_arena* frame_arena;
    window* win;
} _w32_win_data;


typedef struct _win_plat_info {
    HWND window;

    // Used for a reasonable default
    _w32_win_data win_data_no_arena;

    // Used to detect trackpad zoom gestures
    _w32_trackpad_context trackpad_context;
} _win_plat_info;

