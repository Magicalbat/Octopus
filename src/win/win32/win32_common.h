
#define W32_WIN_CLASS_NAME L"OctopusWindow"

#define BT_USE_ARENAS
#define BT_ARENA mem_arena
#define BT_ARENA_GET_POS arena_get_pos
#define BT_ARENA_PUSH(arena, size) arena_push(arena, size, false)
#define BT_ARENA_POP_TO arena_pop_to
#include "better_trackpad.h"

#include <ShellScalingApi.h>

typedef struct _win_plat_info {
    HWND window;
    bt_context bt;
} _win_plat_info;

