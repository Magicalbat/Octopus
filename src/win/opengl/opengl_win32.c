#ifdef PLATFORM_WIN32

typedef const char* WINAPI (wglGetExtensionsStringARB_func)(HDC hdc);
typedef BOOL WINAPI (wglChoosePixelFormatARB_func)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef HGLRC WINAPI (wglCreateContextAttribsARB_func)(HDC hdc, HGLRC hShareContext, const int *attribList);

typedef struct _win_backend {
    HWND window;
    HDC dc;
    HGLRC context;
} _win_backend;

#define X(ret, name, args) static gl_##name##_func name = NULL;
#   include "opengl_funcs_xlist.h"
#undef X

static u64 _w32_win_ticks_per_sec = 1;

static win_key _w32_keymap[256] = { 0 };

static wglChoosePixelFormatARB_func* wglChoosePixelFormatARB = NULL;
static wglCreateContextAttribsARB_func* wglCreateContextAttribsARB = NULL;

static HINSTANCE _w32_instance = NULL;
static const u16* _w32_window_class_name = L"OctopusWindowClass";
static WNDCLASSEXW _w32_window_class = { 0 };

static b32 _w32_first_win = true;

static LRESULT CALLBACK _w32_window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static b32 _w32_get_wgl_funcs(void);
static b32 _w32_register_class(void);
static void _w32_init_keymap(void);
static b32 _w32_win_init(void);

win_window* window_create(mem_arena* arena, u32 width, u32 height, string8 title) {
    if (_w32_first_win) {
        _w32_first_win = false;

        if (!_w32_win_init()) {
            return NULL;
        }
    }

    win_window* win = ARENA_PUSH(arena, win_window);
    win->title = title;
    win->width = width;
    win->height = height;
    win->backend = ARENA_PUSH(arena, _win_backend);

    RECT win_rect = { 0, 0, (i32)width, (i32)height };
    AdjustWindowRect(&win_rect, WS_OVERLAPPEDWINDOW, FALSE);

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    string16 title16 = str16_from_str8(scratch.arena, title, true);
    win->backend->window = CreateWindowW(
        _w32_window_class_name,
        title16.str,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        win_rect.right - win_rect.left, win_rect.bottom - win_rect.top,
        NULL, NULL, _w32_instance, NULL
    );

    arena_scratch_release(scratch);

    if (win->backend->window == NULL) {
        error_emit("Failed to create window");
        return NULL;
    }

    win->backend->dc = GetDC(win->backend->window);

    if (win->backend->dc == NULL) {
        error_emit("Failed to get the device context for the window");
        return NULL;
    }

    // Setting up pixel format
    {
#define WGL_DRAW_TO_WINDOW_ARB                    0x2001
#define WGL_SUPPORT_OPENGL_ARB                    0x2010
#define WGL_DOUBLE_BUFFER_ARB                     0x2011
#define WGL_PIXEL_TYPE_ARB                        0x2013
#define WGL_COLOR_BITS_ARB                        0x2014
#define WGL_DEPTH_BITS_ARB                        0x2022
#define WGL_STENCIL_BITS_ARB                      0x2023
#define WGL_TYPE_RGBA_ARB                         0x202B

        i32 attribs[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
            WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB,     24,
            WGL_DEPTH_BITS_ARB,     0,
            WGL_STENCIL_BITS_ARB,   0,
            0,
        };

        i32 format = 0;
        u32 num_formats = 0;
        if (!wglChoosePixelFormatARB(win->backend->dc, attribs, NULL, 1, &format, &num_formats) ||
            num_formats == 0) {
            error_emit("Failed to get required OpenGL pixel format");
            return NULL;
        }

        PIXELFORMATDESCRIPTOR pixel_desc = { .nSize = sizeof(PIXELFORMATDESCRIPTOR) };
        if (!DescribePixelFormat(win->backend->dc, format, sizeof(PIXELFORMATDESCRIPTOR), &pixel_desc)) {
            error_emit("Failed to describe OpenGL pixel format");
            return NULL;
        }

        if (!SetPixelFormat(win->backend->dc, format, &pixel_desc)) {
            error_emit("Failed to set OpenGL pixel format");
            return NULL;
        }
    }

#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x0001
#define WGL_CONTEXT_FLAGS_ARB                     0x2094
#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x0002

    {
        i32 attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 5,
            WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,

#ifndef NDEBUG
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
            0,
        };

        win->backend->context = wglCreateContextAttribsARB(win->backend->dc, NULL, attribs);

        if (win->backend->context == NULL) {
            error_emit("Failed to create modern OpenGL context (OpenGL 4.5 may not be supported)");
            return NULL;
        }
    }

    if (!wglMakeCurrent(win->backend->dc, win->backend->context)) {
        error_emit("Failed to make OpenGL context current");
        return NULL;
    }

    #define X(ret, name, args) name = (gl_##name##_func)wglGetProcAddress(#name);
    #   include "opengl_funcs_xlist.h"
    #undef X

    // TODO: check functions for NULL?

    SetWindowLongPtrW(win->backend->window, GWLP_USERDATA, (LONG_PTR)win);

    ShowWindow(win->backend->window, SW_SHOW);
    glViewport(0, 0, (i32)win->width, (i32)win->height);

    return win;
}

void window_destroy(win_window* win) { 
    if (win == NULL) {
        return;
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(win->backend->context);
    ReleaseDC(win->backend->window, win->backend->dc);
    DestroyWindow(win->backend->window);
}

b32 window_make_current(win_window* win) { 
    if (win == NULL) {
        return false;
    }

    return wglMakeCurrent(win->backend->dc, win->backend->context);
}

void window_process_events(win_window* win) { 
    if (win == NULL) {
        return;
    }

    // Clearing old event info
    win->num_pen_samples = 0;
    memset(win->pen_samples, 0, sizeof(win_pen_sample) * WIN_PEN_MAX_SAMPLES);

    for (u32 i = 0; i < win->num_touches; i++) {
        if (win->touches[i].cur_flag & WIN_TOUCH_FLAG_JUST_UP) {
            // Remove this touch
            for (u32 j = i; j < win->num_touches-1; j++) {
                win->touches[j] = win->touches[j+1];
            }

            win->num_touches--;
        }

        if (win->touches[i].cur_flag & WIN_TOUCH_FLAG_JUST_DOWN) {
            win->touches[i].cur_flag ^= WIN_TOUCH_FLAG_JUST_DOWN;
        }

        win->touches[i].num_samples = 0;
        memset(win->touches[i].positions, 0, sizeof(vec2f) * WIN_TOUCH_MAX_SAMPLES);
        memset(win->touches[i].flags, 0, sizeof(u8) * WIN_TOUCH_MAX_SAMPLES);
    }

    memcpy(win->prev_mouse_buttons, win->mouse_buttons, WIN_MB_COUNT);
    memcpy(win->prev_keys, win->keys, WIN_KEY_COUNT);

    win->mouse_scroll = 0;

    // Processing events
    MSG msg = { 0 };
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void window_clear_color(win_window* win, vec4f col) { 
    if (win == NULL) {
        return;
    }


    glClearColor(col.x, col.y, col.z, col.w);
}

void window_clear(win_window* win) { 
    if (win == NULL) {
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT);
}

void window_swap_buffers(win_window* win) { 
    if (win == NULL) {
        return;
    }

    SwapBuffers(win->backend->dc);
}

static LRESULT CALLBACK _w32_window_proc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    win_window* win = (win_window*)GetWindowLongPtrW(wnd, GWLP_USERDATA);

    switch (msg) {
        case WM_POINTERDOWN:
        case WM_POINTERUP: 
        case WM_POINTERUPDATE: {
            u32 pointer_id = GET_POINTERID_WPARAM(w_param);
            POINTER_INPUT_TYPE pointer_type = PT_POINTER;

            if (!GetPointerType(pointer_id, &pointer_type)) {
                break;
            }

            if (pointer_type == PT_PEN) {
                if (win->num_pen_samples >= WIN_PEN_MAX_SAMPLES) {
                    break;
                }

                POINTER_PEN_INFO pen_info = { 0 };

                if (!GetPointerPenInfo(pointer_id, &pen_info)) {
                    break;
                }

                mem_arena_temp scratch = arena_scratch_get(NULL, 0);

                u32 history_count = pen_info.pointerInfo.historyCount;

                POINTER_PEN_INFO* pen_infos = ARENA_PUSH_ARRAY(
                    scratch.arena,
                    POINTER_PEN_INFO,
                    history_count
                );

                if (
                    !GetPointerPenInfoHistory(pointer_id, &history_count, pen_infos) ||
                    history_count != pen_info.pointerInfo.historyCount
                ) {
                    arena_scratch_release(scratch);
                    break;
                }

                if (win->num_pen_samples + history_count > WIN_PEN_MAX_SAMPLES) {
                    history_count = WIN_PEN_MAX_SAMPLES - win->num_pen_samples;
                }

                for (i32 i = (i32)history_count-1; i >= 0; i--) {
                    win_pen_sample cur_sample = { 
                        .pressure = 1,

                        .time_us = pen_infos[i].pointerInfo.PerformanceCount *
                            1000000 / _w32_win_ticks_per_sec
                    };

                    POINT win_pos = pen_infos[i].pointerInfo.ptPixelLocation;
                    ScreenToClient(win->backend->window, &win_pos);

                    cur_sample.pos.x = (f32)win_pos.x;
                    cur_sample.pos.y = (f32)win_pos.y;

                    if (pen_infos[i].penMask & PEN_MASK_PRESSURE) {
                        cur_sample.pressure = (f32)pen_infos[i].pressure / 1024;
                    }
                    if (pen_infos[i].penMask & PEN_MASK_ROTATION) {
                        cur_sample.rotation = (f32)pen_infos[i].rotation / 180.0f * (f32)PI;
                    }
                    if (pen_infos[i].penMask & PEN_MASK_TILT_X) {
                        cur_sample.tilt.x = (f32)pen_infos[i].tiltX / 180.0f * (f32)PI;
                    }
                    if (pen_infos[i].penMask & PEN_MASK_TILT_Y) {
                        cur_sample.tilt.y = (f32)pen_infos[i].tiltY / 180.0f * (f32)PI;
                    }

                    if (pen_infos[i].penFlags & PEN_FLAG_ERASER)
                        cur_sample.flags |= WIN_PEN_FLAG_ERASER;

                    POINTER_FLAGS flags = pen_infos[i].pointerInfo.pointerFlags;

                    b32 in_range = flags & POINTER_FLAG_INRANGE;
                    b32 in_contact = flags & POINTER_FLAG_INCONTACT;

                    if (in_range && !in_contact)
                        cur_sample.flags |= WIN_PEN_FLAG_HOVERING;
                    if (in_contact)
                        cur_sample.flags |= WIN_PEN_FLAG_DOWN;
                    if (flags & POINTER_FLAG_DOWN)
                        cur_sample.flags |= WIN_PEN_FLAG_JUST_DOWN;
                    if (flags & POINTER_FLAG_UP)
                        cur_sample.flags |= WIN_PEN_FLAG_JUST_UP;

                    win->pen_samples[win->num_pen_samples++] = cur_sample;
                }

                win->last_pen_sample = win->pen_samples[win->num_pen_samples-1];

                arena_scratch_release(scratch);
            } else if (pointer_type == PT_TOUCH) {
                POINTER_INFO pointer_info = { 0 };
                
                if (!GetPointerInfo(pointer_id, &pointer_info)) {
                    break;
                }

                i32 touch_index = -1;
                for (u32 i = 0; i < win->num_touches; i++) {
                    if (win->touches[i].id == pointer_id) {
                        touch_index = (i32)i;
                        break;
                    }
                }

                if (touch_index == -1) {
                    if (win->num_touches >= WIN_MAX_TOUCHES) {
                        break;
                    }

                    touch_index = (i32)(win->num_touches++);
                    win->touches[touch_index].id = pointer_id;
                }

                win_touch_info* cur_touch = &win->touches[touch_index];

                if (cur_touch->num_samples >= WIN_TOUCH_MAX_SAMPLES) {
                    break;
                }

                u32 history_count = pointer_info.historyCount;

                mem_arena_temp scratch = arena_scratch_get(NULL, 0);

                POINTER_INFO* infos = ARENA_PUSH_ARRAY(scratch.arena, POINTER_INFO, history_count);

                if (
                    !GetPointerInfoHistory(pointer_id, &history_count, infos) ||
                    history_count != pointer_info.historyCount
                ) {
                    arena_scratch_release(scratch);
                    break;
                }

                if (cur_touch->num_samples + history_count > WIN_TOUCH_MAX_SAMPLES) {
                    history_count = WIN_TOUCH_MAX_SAMPLES - cur_touch->num_samples;
                }

                for (i32 i = (i32)history_count - 1; i >= 0; i--) {
                    POINT win_pos = infos[i].ptPixelLocation;
                    ScreenToClient(win->backend->window, &win_pos);

                    vec2f cur_pos = { (f32)win_pos.x, (f32)win_pos.y };

                    u8 cur_flags = 0;

                    if (infos[i].pointerFlags & POINTER_FLAG_INCONTACT)
                        cur_flags |= WIN_TOUCH_FLAG_DOWN;
                    if (infos[i].pointerFlags & POINTER_FLAG_DOWN) 
                        cur_flags |= WIN_TOUCH_FLAG_JUST_DOWN;
                    if (infos[i].pointerFlags & POINTER_FLAG_UP)
                        cur_flags |= WIN_TOUCH_FLAG_JUST_UP;

                    u32 index = cur_touch->num_samples++;

                    cur_touch->positions[index] = cur_pos;
                    cur_touch->flags[index] = cur_flags;
                }

                cur_touch->cur_pos = cur_touch->positions[cur_touch->num_samples-1];
                cur_touch->cur_flag = cur_touch->flags[cur_touch->num_samples-1];

                arena_scratch_release(scratch);
            }
        } break;

        case WM_MOUSEMOVE: {
            win->mouse_pos.x = (f32)((l_param) & 0xffff);
            win->mouse_pos.y = (f32)((l_param >> 16) & 0xffff);
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
            win->mouse_scroll = SIGN(GET_WHEEL_DELTA_WPARAM(w_param));
        } break;

        case WM_KEYDOWN: {
            win_key down_key = _w32_keymap[w_param];
            win->keys[down_key] = true;
        } break;
        case WM_KEYUP: {
            win_key up_key = _w32_keymap[w_param];
            win->keys[up_key] = false;
        } break;

        case WM_SIZE: {
            u32 width = (u32)LOWORD(l_param);
            u32 height = (u32)HIWORD(l_param);

            win->width = width;
            win->height = height;

            glViewport(0, 0, (i32)width, (i32)height);
        } break;

        case WM_CLOSE: {
            win->should_close = true;
        } break;
        default: break;
    }

    return DefWindowProcW(wnd, msg, w_param, l_param);
}

static b32 _w32_get_wgl_funcs(void) {
    HWND dummy_window = CreateWindowW(
        L"STATIC", L"Dummy Window", WS_OVERLAPPED,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, NULL, NULL
    );

    if (dummy_window == NULL) {
        error_emit("Failed to create dummy window for WGL functions"); 
        return false;
    }

    HDC dc = GetDC(dummy_window);

    if (dc == NULL) {
        error_emit("Failed to get device context for dummy window for WGL functions"); 
        return false;
    }

    PIXELFORMATDESCRIPTOR pixel_desc = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 24,
    };

    i32 format = ChoosePixelFormat(dc, &pixel_desc);

    if (format == 0) {
        error_emit("Failed to choose pixel format for dummy window for WGL functions"); 
        return false;
    }

    if (!DescribePixelFormat(dc, format, sizeof(PIXELFORMATDESCRIPTOR), &pixel_desc)) {
        error_emit("Failed to describe pixel format for dummy window for WGL functions"); 
        return false;
    }

    if (!SetPixelFormat(dc, format, &pixel_desc)) {
        error_emit("Failed to set pixel format for dummy window for WGL functions"); 
        return false;
    }

    HGLRC context = wglCreateContext(dc);

    if (context == NULL) {
        error_emit("Failed to create OpenGL context for dummy window for WGL functions"); 
        return false;
    }

    if (!wglMakeCurrent(dc, context)) {
        error_emit("Failed to make OpenGL context current for dummy window for WGL functions"); 
        return false;
    }

    wglGetExtensionsStringARB_func* wglGetExtensionsStringARB =
        (wglGetExtensionsStringARB_func*)((void*)wglGetProcAddress("wglGetExtensionsStringARB"));

    if (wglGetExtensionsStringARB == NULL) {
        error_emit("OpenGL does not support WGL_ARB_extensions_string extension"); 
        return false;
    }

    const char* extensions = wglGetExtensionsStringARB(dc);
    string8 ext_str = str8_from_cstr((u8*)extensions);

    while (ext_str.size) {
        u64 index = str8_index_char(ext_str, (u8)' ');
        string8 cur_str = str8_substr(ext_str, 0, index);
        ext_str = str8_substr(ext_str, index + 1, ext_str.size);

        if (str8_equals(cur_str, STR8_LIT("WGL_ARB_pixel_format"))) {
            wglChoosePixelFormatARB =
                (wglChoosePixelFormatARB_func*)((void*)wglGetProcAddress("wglChoosePixelFormatARB"));
        } else if (str8_equals(cur_str, STR8_LIT("WGL_ARB_create_context"))) {
            wglCreateContextAttribsARB =
                (wglCreateContextAttribsARB_func*)((void*)wglGetProcAddress("wglCreateContextAttribsARB"));
        }
    }

    if (wglChoosePixelFormatARB == NULL || wglCreateContextAttribsARB == NULL) {
        error_emit("WGL extensions required for modern contexts are not supported"); 
        return false;
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(context);
    ReleaseDC(dummy_window, dc);
    DestroyWindow(dummy_window);

    return true;
}

static b32 _w32_register_class(void) {
    _w32_window_class = (WNDCLASSEXW) {
        .cbSize = sizeof(WNDCLASSEXW),
        .lpfnWndProc = _w32_window_proc,
        .hInstance = _w32_instance,
        .hCursor = LoadCursorW(NULL, IDC_ARROW),
        .lpszClassName = _w32_window_class_name
    };

    ATOM atom = RegisterClassExW(&_w32_window_class);

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

static b32 _w32_win_init(void) {
    _w32_instance = GetModuleHandleW(NULL);

    if (_w32_instance == NULL) {
        error_emit("Failed to get module handle");
        return false;
    }

    LARGE_INTEGER perf_freq = { 0 };
    if (QueryPerformanceFrequency(&perf_freq)) {
        _w32_win_ticks_per_sec = (u64)perf_freq.QuadPart;
    }

    // TODO: make helpers to account for scaling
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        error_emit("Failed to set DPI awareness context");
        return false;
    }

    b32 ret = true;

    ret *= _w32_get_wgl_funcs();
    ret *= _w32_register_class();
    _w32_init_keymap();

    return ret;
}

#endif // PLATFORM_WIN32
