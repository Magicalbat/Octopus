
#define BT_IMPLEMENTATION
#include "better_trackpad.h"

static b32 _w32_win_initialized = false;
static win_key _w32_keymap[256];

static LRESULT CALLBACK _w32_window_proc(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam
);

static void _w32_update_win_dpi(window* win);
static b32 _w32_register_win_class(void);

window* win_create(mem_arena* arena, u32 width, u32 height, string8 title) {
    if (!_w32_win_initialized) {
        if (!SetProcessDpiAwarenessContext(
            DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
        )) {
            warn_emit("Failed to set DPI awareness context");
        }

        if (!EnableMouseInPointer(true)) {
            warn_emit("Failed to enable mouse in pointer");
        }

        _w32_win_initialized = _w32_register_win_class();
    }

    if (!_w32_win_initialized) {
        return NULL;
    }

    mem_arena_temp maybe_temp = arena_temp_begin(arena);
    window* win = PUSH_STRUCT(maybe_temp.arena, window);

    win->width = width;
    win->height = height;
    win->plat_info = PUSH_STRUCT(maybe_temp.arena, _win_plat_info);

    RECT win_rect = { 0, 0, (i32)width, (i32)height };
    if (!AdjustWindowRect(&win_rect, WS_OVERLAPPEDWINDOW, FALSE)) {
        error_emit("Failed to adjust window rect");
        goto fail;
    }

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    string16 title16 = str16_from_str8(scratch.arena, title, true);
    win->plat_info->window = CreateWindowW(
        W32_WIN_CLASS_NAME,
        title16.str,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        win_rect.right - win_rect.left, win_rect.bottom - win_rect.top,
        NULL, NULL, NULL, NULL
    );
    arena_scratch_release(scratch);

    if (win->plat_info->window == NULL) {
        error_emit("Failed to create window");
        goto fail;
    }

    bt_init(arena, &win->plat_info->bt, &(bt_context_desc){
        .hwnd = win->plat_info->window,
        .flags.disable_raw_msgs = 1,
        .flags.disable_contact_msgs = 1,
    });

    if (!win->plat_info->bt.initialized) {
        warn_emitf(
            "Failed to init better trackpad: %s",
            win->plat_info->bt.err_str
        );
    }

    SetWindowLongPtrW(win->plat_info->window, GWLP_USERDATA, (LONG_PTR)win);

    if (!_win_equip_gfx(arena, win)) {
        goto fail;
    }

    _w32_update_win_dpi(win);

    ShowWindow(win->plat_info->window, SW_SHOW);

    return win;

fail:
    if (win->plat_info->window != NULL) {
        DestroyWindow(win->plat_info->window);
    }

    arena_temp_end(maybe_temp);
    return NULL;
}

void win_destroy(window* win) {
    if (win == NULL) { return; }

    _win_unequip_gfx(win);
    DestroyWindow(win->plat_info->window);
}

void win_process_events(window* win) {
    memcpy(win->prev_mouse_buttons, win->mouse_buttons, WIN_MB_COUNT);
    memcpy(win->prev_keys, win->keys, WIN_KEY_COUNT);

    win->mouse_scroll = (v2_f32){ 0, 0 };
    win->touchpad_zoom = 1.0f;

    bt_passive_update(&win->plat_info->bt, win->plat_info->window);

    // Processing events
    MSG msg = { 0 };
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

static LRESULT CALLBACK _w32_window_proc(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam
) {
    window* win = (window*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch (uMsg) {
        case WM_INPUT: {
            bt_process_input(&win->plat_info->bt, win->plat_info->window, wParam, lParam);
        } break;

        case BT_MSG_GESTURE_ZOOM: {
            bt_gesture_zoom zoom = BT_GESTURE_ZOOM_LPARAM(lParam);
            win->touchpad_zoom *= 1.0f - zoom.dist_change / zoom.start_dist;
        } break;

        case WM_MOUSEMOVE: {
            win->mouse_pos.x = (f32)((lParam) & 0xffff);
            win->mouse_pos.y = (f32)((lParam >> 16) & 0xffff);
        } break;

        case WM_LBUTTONDOWN: { win->mouse_buttons[WIN_MB_LEFT] = true; } break;
        case WM_LBUTTONUP: { win->mouse_buttons[WIN_MB_LEFT] = false; } break;
        case WM_MBUTTONDOWN: { win->mouse_buttons[WIN_MB_MIDDLE] = true; } break;
        case WM_MBUTTONUP: { win->mouse_buttons[WIN_MB_MIDDLE] = false; } break;
        case WM_RBUTTONDOWN: { win->mouse_buttons[WIN_MB_RIGHT] = true; } break;
        case WM_RBUTTONUP: { win->mouse_buttons[WIN_MB_RIGHT] = false; } break;

        case WM_MOUSEWHEEL: {
            if (win->plat_info->bt.gesture_state != BT_GES_ZOOM) {
                f32 delta = (f32)GET_WHEEL_DELTA_WPARAM(wParam);
                win->mouse_scroll.y += delta / (f32)WHEEL_DELTA;;
            }
        } break;

        case WM_MOUSEHWHEEL: {
            f32 delta = (f32)GET_WHEEL_DELTA_WPARAM(wParam);
            win->mouse_scroll.x += delta / (f32)WHEEL_DELTA;
        } break;

        case WM_KEYDOWN: {
            win_key down_key = wParam <= sizeof(_w32_keymap) /
                sizeof(_w32_keymap[0]) ? _w32_keymap[wParam] : WIN_KEY_NONE;
            win->keys[down_key] = true;
        } break;

        case WM_KEYUP: {
            win_key up_key = wParam <= sizeof(_w32_keymap) /
                sizeof(_w32_keymap[0]) ? _w32_keymap[wParam] : WIN_KEY_NONE;
            win->keys[up_key] = false;
        } break;

        case WM_DPICHANGED: {
            _w32_update_win_dpi(win);

            const RECT* new_win_rect = (RECT*)lParam;
            SetWindowPos(
                win->plat_info->window, NULL,
                new_win_rect->left, new_win_rect->top,
                new_win_rect->right - new_win_rect->left,
                new_win_rect->bottom - new_win_rect->top,
                SWP_NOZORDER | SWP_NOACTIVATE
            );
        } break;

        case WM_SIZE: {
            u32 width = (u32)LOWORD(lParam);
            u32 height = (u32)HIWORD(lParam);

            win->width = width;
            win->height = height;
        } break;

        case WM_CLOSE: {
            win->flags |= WIN_FLAG_SHOULD_CLOSE;
        } break;

        default: {
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
        } break;
    }

    return 0;
}

static void _w32_update_win_dpi(window* win) {
    HMONITOR monitor = MonitorFromWindow(
        win->plat_info->window,
        MONITOR_DEFAULTTONEAREST
    );

    u32 raw_dpi = 96;
    GetDpiForMonitor(monitor, MDT_RAW_DPI, &raw_dpi, &raw_dpi);
    u32 effective_dpi = 96;
    GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &effective_dpi, &effective_dpi);

    win->dpi = effective_dpi;
    win->raw_dpi = raw_dpi;
}

static b32 _w32_register_win_class(void) {
    HINSTANCE module_handle = GetModuleHandleW(NULL);

    if (module_handle == NULL) {
        error_emit("Failed to get module handle");
        return false; 
    }

    WNDCLASSW wnd_class = (WNDCLASSW){
        .lpfnWndProc = _w32_window_proc,
        .hInstance = module_handle,
        .hCursor = LoadCursorW(NULL, IDC_ARROW),
        .lpszClassName = W32_WIN_CLASS_NAME 
    };

    ATOM atom = RegisterClassW(&wnd_class);

    if (!atom) {
        error_emit("Failed to register window class");
        return false;
    }

    return true;
}

static win_key _w32_keymap[256] = {
    [VK_BACK] = WIN_KEY_BACKSPACE,
    [VK_TAB] = WIN_KEY_TAB,
    [VK_RETURN] = WIN_KEY_ENTER,
    [VK_CAPITAL] = WIN_KEY_CAPSLOCK,
    [VK_ESCAPE] = WIN_KEY_ESCAPE,
    [VK_SPACE] = WIN_KEY_SPACE,
    [VK_PRIOR] = WIN_KEY_PAGEUP,
    [VK_NEXT] = WIN_KEY_PAGEDOWN,
    [VK_END] = WIN_KEY_END,
    [VK_HOME] = WIN_KEY_HOME,
    [VK_LEFT] = WIN_KEY_ARROW_LEFT,
    [VK_UP] = WIN_KEY_ARROW_UP,
    [VK_RIGHT] = WIN_KEY_ARROW_RIGHT,
    [VK_DOWN] = WIN_KEY_ARROW_DOWN,
    [VK_INSERT] = WIN_KEY_INSERT,
    [VK_DELETE] = WIN_KEY_DELETE,
    ['0'] = WIN_KEY_0,
    ['1'] = WIN_KEY_1,
    ['2'] = WIN_KEY_2,
    ['3'] = WIN_KEY_3,
    ['4'] = WIN_KEY_4,
    ['5'] = WIN_KEY_5,
    ['6'] = WIN_KEY_6,
    ['7'] = WIN_KEY_7,
    ['8'] = WIN_KEY_8,
    ['9'] = WIN_KEY_9,
    ['A'] = WIN_KEY_A,
    ['B'] = WIN_KEY_B,
    ['C'] = WIN_KEY_C,
    ['D'] = WIN_KEY_D,
    ['E'] = WIN_KEY_E,
    ['F'] = WIN_KEY_F,
    ['G'] = WIN_KEY_G,
    ['H'] = WIN_KEY_H,
    ['I'] = WIN_KEY_I,
    ['J'] = WIN_KEY_J,
    ['K'] = WIN_KEY_K,
    ['L'] = WIN_KEY_L,
    ['M'] = WIN_KEY_M,
    ['N'] = WIN_KEY_N,
    ['O'] = WIN_KEY_O,
    ['P'] = WIN_KEY_P,
    ['Q'] = WIN_KEY_Q,
    ['R'] = WIN_KEY_R,
    ['S'] = WIN_KEY_S,
    ['T'] = WIN_KEY_T,
    ['U'] = WIN_KEY_U,
    ['V'] = WIN_KEY_V,
    ['W'] = WIN_KEY_W,
    ['X'] = WIN_KEY_X,
    ['Y'] = WIN_KEY_Y,
    ['Z'] = WIN_KEY_Z,
    [VK_NUMPAD0] = WIN_KEY_NUMPAD_0,
    [VK_NUMPAD1] = WIN_KEY_NUMPAD_1,
    [VK_NUMPAD2] = WIN_KEY_NUMPAD_2,
    [VK_NUMPAD3] = WIN_KEY_NUMPAD_3,
    [VK_NUMPAD4] = WIN_KEY_NUMPAD_4,
    [VK_NUMPAD5] = WIN_KEY_NUMPAD_5,
    [VK_NUMPAD6] = WIN_KEY_NUMPAD_6,
    [VK_NUMPAD7] = WIN_KEY_NUMPAD_7,
    [VK_NUMPAD8] = WIN_KEY_NUMPAD_8,
    [VK_NUMPAD9] = WIN_KEY_NUMPAD_9,
    [VK_MULTIPLY] = WIN_KEY_NUMPAD_MULTIPLY,
    [VK_ADD] = WIN_KEY_NUMPAD_ADD,
    [VK_SUBTRACT] = WIN_KEY_NUMPAD_SUBTRACT,
    [VK_DECIMAL] = WIN_KEY_NUMPAD_DECIMAL,
    [VK_DIVIDE] = WIN_KEY_NUMPAD_DIVIDE,
    [VK_F1] = WIN_KEY_F1,
    [VK_F2] = WIN_KEY_F2,
    [VK_F3] = WIN_KEY_F3,
    [VK_F4] = WIN_KEY_F4,
    [VK_F5] = WIN_KEY_F5,
    [VK_F6] = WIN_KEY_F6,
    [VK_F7] = WIN_KEY_F7,
    [VK_F8] = WIN_KEY_F8,
    [VK_F9] = WIN_KEY_F9,
    [VK_F10] = WIN_KEY_F10,
    [VK_F11] = WIN_KEY_F11,
    [VK_F12] = WIN_KEY_F12,
    [VK_NUMLOCK] = WIN_KEY_NUM_LOCK,
    [VK_SCROLL] = WIN_KEY_SCROLL_LOCK,
    [VK_SHIFT] = WIN_KEY_SHIFT,
    [VK_CONTROL] = WIN_KEY_CONTROL,
    [VK_MENU] = WIN_KEY_ALT,
    [VK_LSHIFT] = WIN_KEY_SHIFT,
    [VK_RSHIFT] = WIN_KEY_SHIFT,
    [VK_LCONTROL] = WIN_KEY_CONTROL,
    [VK_RCONTROL] = WIN_KEY_CONTROL,
    [VK_LMENU] = WIN_KEY_ALT,
    [VK_RMENU] = WIN_KEY_ALT,
    [VK_OEM_1] = WIN_KEY_SEMICOLON,
    [VK_OEM_PLUS] = WIN_KEY_EQUAL,
    [VK_OEM_COMMA] = WIN_KEY_COMMA,
    [VK_OEM_MINUS] = WIN_KEY_MINUS,
    [VK_OEM_PERIOD] = WIN_KEY_PERIOD,
    [VK_OEM_2] = WIN_KEY_FORWARDSLASH,
    [VK_OEM_3] = WIN_KEY_BACKTICK,
    [VK_OEM_4] = WIN_KEY_LBRACKET,
    [VK_OEM_5] = WIN_KEY_BACKSLASH,
    [VK_OEM_6] = WIN_KEY_RBRACKET,
    [VK_OEM_7] = WIN_KEY_APOSTROPHE,
};

