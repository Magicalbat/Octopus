
static b32 _w32_win_initialized = false;
static win_key _w32_keymap[256] = { 0 };
static const u16* _w32_win_class_name = L"OctopusWindow";

static LRESULT CALLBACK _w32_window_proc(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam
);

static b32 _w32_register_win_class(void);
static void _w32_init_keymap(void);

window* win_create(mem_arena* arena, u32 width, u32 height, string8 title) {
    if (!_w32_win_initialized) {
        if (!SetProcessDpiAwarenessContext(
            DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
        )) {
            warn_emit("Failed to set DPI awareness context");
        }

        _w32_init_keymap();
        _w32_win_initialized = _w32_register_win_class();
    }

    mem_arena_temp maybe_temp = arena_temp_begin(arena);
    window* win = PUSH_STRUCT(maybe_temp.arena, window);

    win->width = width;
    win->height = height;
    win->plat_backend = PUSH_STRUCT(maybe_temp.arena, _win_plat_backend);

    RECT win_rect = { 0, 0, (i32)width, (i32)height };
    if (!AdjustWindowRect(&win_rect, WS_OVERLAPPEDWINDOW, FALSE)) {
        error_emit("Failed to adjust window rect");
        goto fail;
    }

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    string16 title16 = str16_from_str8(scratch.arena, title, true);
    win->plat_backend->window = CreateWindowW(
        _w32_win_class_name,
        title16.str,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        win_rect.right - win_rect.left, win_rect.bottom - win_rect.top,
        NULL, NULL, NULL, NULL
    );
    arena_scratch_release(scratch);

    if (win->plat_backend->window == NULL) {
        error_emit("Failed to create window");
        goto fail;
    }

    /*if (!_win_create_graphics(maybe_temp.arena, win)) {
        goto fail;
    }*/

    SetWindowLongPtrW(win->plat_backend->window, GWLP_USERDATA, (LONG_PTR)win);

    ShowWindow(win->plat_backend->window, SW_SHOW);

    return win;

fail:
    if (win->plat_backend->window != NULL) {
        DestroyWindow(win->plat_backend->window);
    }

    arena_temp_end(maybe_temp);
    return NULL;
}

void win_destroy(window* win) {
    if (win == NULL) { return; }

    //_win_destroy_graphics(win);
    DestroyWindow(win->plat_backend->window);
}

void win_process_events(window* win) {
    memcpy(win->prev_mouse_buttons, win->mouse_buttons, WIN_MB_COUNT);
    memcpy(win->prev_keys, win->keys, WIN_KEY_COUNT);

    win->mouse_scroll = (vec2f){ 0, 0 };

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
        case WM_MOUSEMOVE: {
            win->mouse_pos.x = (f32)((lParam) & 0xffff);
            win->mouse_pos.y = (f32)((lParam >> 16) & 0xffff);
        } break;

        case WM_LBUTTONDOWN: {
            win->mouse_buttons[WIN_MB_LEFT] = true;
        } break;
        case WM_LBUTTONUP: {
            win->mouse_buttons[WIN_MB_LEFT] = false;
        } break;
        case WM_MBUTTONDOWN: {
            win->mouse_buttons[WIN_MB_MIDDLE] = true;
        } break;
        case WM_MBUTTONUP: {
            win->mouse_buttons[WIN_MB_MIDDLE] = false;
        } break;
        case WM_RBUTTONDOWN: {
            win->mouse_buttons[WIN_MB_RIGHT] = true;
        } break;
        case WM_RBUTTONUP: {
            win->mouse_buttons[WIN_MB_RIGHT] = false;
        } break;

        case WM_MOUSEWHEEL: {
            f32 delta = (f32)GET_WHEEL_DELTA_WPARAM(wParam);
            win->mouse_scroll.y = delta / (f32)WHEEL_DELTA;
        } break;

        case WM_MOUSEHWHEEL: {
            f32 delta = (f32)GET_WHEEL_DELTA_WPARAM(wParam);
            win->mouse_scroll.x = delta / (f32)WHEEL_DELTA;
        } break;

        case WM_KEYDOWN: {
            win_key down_key = _w32_keymap[wParam];
            win->keys[down_key] = true;
        } break;
        case WM_KEYUP: {
            win_key up_key = _w32_keymap[wParam];
            win->keys[up_key] = false;
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
        default: break;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

static b32 _w32_register_win_class(void) {
    HINSTANCE module_handle = GetModuleHandleW(NULL);

    if (module_handle == NULL) {
        error_emit("Failed to get module handle");
        return false; 
    }

    WNDCLASSEXW win_class = (WNDCLASSEXW) {
        .cbSize = sizeof(WNDCLASSEXW),
        .lpfnWndProc = _w32_window_proc,
        .hInstance = module_handle,
        .hCursor = LoadCursorW(NULL, IDC_ARROW),
        .lpszClassName = _w32_win_class_name
    };

    ATOM atom = RegisterClassExW(&win_class);

    if (!atom) {
        error_emit("Failed to register window class");
        return false;
    }

    return true;
}

static void _w32_init_keymap(void) {
    _w32_keymap[VK_BACK] = WIN_KEY_BACKSPACE;
    _w32_keymap[VK_TAB] = WIN_KEY_TAB;
    _w32_keymap[VK_RETURN] = WIN_KEY_ENTER;
    _w32_keymap[VK_CAPITAL] = WIN_KEY_CAPSLOCK;
    _w32_keymap[VK_ESCAPE] = WIN_KEY_ESCAPE;
    _w32_keymap[VK_SPACE] = WIN_KEY_SPACE;
    _w32_keymap[VK_PRIOR] = WIN_KEY_PAGEUP;
    _w32_keymap[VK_NEXT] = WIN_KEY_PAGEDOWN;
    _w32_keymap[VK_END] = WIN_KEY_END;
    _w32_keymap[VK_HOME] = WIN_KEY_HOME;
    _w32_keymap[VK_LEFT] = WIN_KEY_ARROW_LEFT;
    _w32_keymap[VK_UP] = WIN_KEY_ARROW_UP;
    _w32_keymap[VK_RIGHT] = WIN_KEY_ARROW_RIGHT;
    _w32_keymap[VK_DOWN] = WIN_KEY_ARROW_DOWN;
    _w32_keymap[VK_INSERT] = WIN_KEY_INSERT;
    _w32_keymap[VK_DELETE] = WIN_KEY_DELETE;
    _w32_keymap[0x30] = WIN_KEY_0;
    _w32_keymap[0x31] = WIN_KEY_1;
    _w32_keymap[0x32] = WIN_KEY_2;
    _w32_keymap[0x33] = WIN_KEY_3;
    _w32_keymap[0x34] = WIN_KEY_4;
    _w32_keymap[0x35] = WIN_KEY_5;
    _w32_keymap[0x36] = WIN_KEY_6;
    _w32_keymap[0x37] = WIN_KEY_7;
    _w32_keymap[0x38] = WIN_KEY_8;
    _w32_keymap[0x39] = WIN_KEY_9;
    _w32_keymap[0x41] = WIN_KEY_A;
    _w32_keymap[0x42] = WIN_KEY_B;
    _w32_keymap[0x43] = WIN_KEY_C;
    _w32_keymap[0x44] = WIN_KEY_D;
    _w32_keymap[0x45] = WIN_KEY_E;
    _w32_keymap[0x46] = WIN_KEY_F;
    _w32_keymap[0x47] = WIN_KEY_G;
    _w32_keymap[0x48] = WIN_KEY_H;
    _w32_keymap[0x49] = WIN_KEY_I;
    _w32_keymap[0x4A] = WIN_KEY_J;
    _w32_keymap[0x4B] = WIN_KEY_K;
    _w32_keymap[0x4C] = WIN_KEY_L;
    _w32_keymap[0x4D] = WIN_KEY_M;
    _w32_keymap[0x4E] = WIN_KEY_N;
    _w32_keymap[0x4F] = WIN_KEY_O;
    _w32_keymap[0x50] = WIN_KEY_P;
    _w32_keymap[0x51] = WIN_KEY_Q;
    _w32_keymap[0x52] = WIN_KEY_R;
    _w32_keymap[0x53] = WIN_KEY_S;
    _w32_keymap[0x54] = WIN_KEY_T;
    _w32_keymap[0x55] = WIN_KEY_U;
    _w32_keymap[0x56] = WIN_KEY_V;
    _w32_keymap[0x57] = WIN_KEY_W;
    _w32_keymap[0x58] = WIN_KEY_X;
    _w32_keymap[0x59] = WIN_KEY_Y;
    _w32_keymap[0x5A] = WIN_KEY_Z;
    _w32_keymap[VK_NUMPAD0] = WIN_KEY_NUMPAD_0;
    _w32_keymap[VK_NUMPAD1] = WIN_KEY_NUMPAD_1;
    _w32_keymap[VK_NUMPAD2] = WIN_KEY_NUMPAD_2;
    _w32_keymap[VK_NUMPAD3] = WIN_KEY_NUMPAD_3;
    _w32_keymap[VK_NUMPAD4] = WIN_KEY_NUMPAD_4;
    _w32_keymap[VK_NUMPAD5] = WIN_KEY_NUMPAD_5;
    _w32_keymap[VK_NUMPAD6] = WIN_KEY_NUMPAD_6;
    _w32_keymap[VK_NUMPAD7] = WIN_KEY_NUMPAD_7;
    _w32_keymap[VK_NUMPAD8] = WIN_KEY_NUMPAD_8;
    _w32_keymap[VK_NUMPAD9] = WIN_KEY_NUMPAD_9;
    _w32_keymap[VK_MULTIPLY] = WIN_KEY_NUMPAD_MULTIPLY;
    _w32_keymap[VK_ADD] = WIN_KEY_NUMPAD_ADD;
    _w32_keymap[VK_SUBTRACT] = WIN_KEY_NUMPAD_SUBTRACT;
    _w32_keymap[VK_DECIMAL] = WIN_KEY_NUMPAD_DECIMAL;
    _w32_keymap[VK_DIVIDE] = WIN_KEY_NUMPAD_DIVIDE;
    _w32_keymap[VK_F1] = WIN_KEY_F1;
    _w32_keymap[VK_F2] = WIN_KEY_F2;
    _w32_keymap[VK_F3] = WIN_KEY_F3;
    _w32_keymap[VK_F4] = WIN_KEY_F4;
    _w32_keymap[VK_F5] = WIN_KEY_F5;
    _w32_keymap[VK_F6] = WIN_KEY_F6;
    _w32_keymap[VK_F7] = WIN_KEY_F7;
    _w32_keymap[VK_F8] = WIN_KEY_F8;
    _w32_keymap[VK_F9] = WIN_KEY_F9;
    _w32_keymap[VK_F10] = WIN_KEY_F10;
    _w32_keymap[VK_F11] = WIN_KEY_F11;
    _w32_keymap[VK_F12] = WIN_KEY_F12;
    _w32_keymap[VK_NUMLOCK] = WIN_KEY_NUM_LOCK;
    _w32_keymap[VK_SCROLL] = WIN_KEY_SCROLL_LOCK;
    _w32_keymap[VK_LSHIFT] = WIN_KEY_LSHIFT;
    _w32_keymap[VK_RSHIFT] = WIN_KEY_RSHIFT;
    _w32_keymap[VK_LCONTROL] = WIN_KEY_LCONTROL;
    _w32_keymap[VK_RCONTROL] = WIN_KEY_RCONTROL;
    _w32_keymap[VK_LMENU] = WIN_KEY_LALT;
    _w32_keymap[VK_RMENU] = WIN_KEY_RALT;
    _w32_keymap[VK_OEM_1] = WIN_KEY_SEMICOLON;
    _w32_keymap[VK_OEM_PLUS] = WIN_KEY_EQUAL;
    _w32_keymap[VK_OEM_COMMA] = WIN_KEY_COMMA;
    _w32_keymap[VK_OEM_MINUS] = WIN_KEY_MINUS;
    _w32_keymap[VK_OEM_PERIOD] = WIN_KEY_PERIOD;
    _w32_keymap[VK_OEM_2] = WIN_KEY_FORWARDSLASH;
    _w32_keymap[VK_OEM_3] = WIN_KEY_BACKTICK;
    _w32_keymap[VK_OEM_4] = WIN_KEY_LBRACKET;
    _w32_keymap[VK_OEM_5] = WIN_KEY_BACKSLASH;
    _w32_keymap[VK_OEM_6] = WIN_KEY_RBRACKET;
    _w32_keymap[VK_OEM_7] = WIN_KEY_APOSTROPHE;
}

