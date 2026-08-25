
#define W32_WIN_CLASS_NAME L"OctopusWindow"

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
} _win_plat_info;

