#ifdef PLATFORM_WIN32

typedef const char* WINAPI (wglGetExtensionsStringARB_func)(HDC hdc);
typedef BOOL WINAPI (wglChoosePixelFormatARB_func)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef HGLRC WINAPI (wglCreateContextAttribsARB_func)(HDC hdc, HGLRC hShareContext, const int *attribList);

typedef struct _gfx_win_backend {
    HWND window;
    HDC dc;
    HGLRC context;
} _gfx_win_backend;

#define X(ret, name, args) gl_##name##_func name = NULL;
#   include "opengl_funcs_xlist.h"
#undef X

static u32 _w32_keymap[256] = { 0 };

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
static b32 _w32_gfx_init(void);

gfx_window* gfx_win_create(mem_arena* arena, u32 width, u32 height, string8 title) {
    if (_w32_first_win) {
        _w32_first_win = false;

        if (!_w32_gfx_init()) {
            return NULL;
        }
    }

    gfx_window* win = ARENA_PUSH(arena, gfx_window);
    win->title = title;
    win->width = width;
    win->height = height;
    win->backend = ARENA_PUSH(arena, _gfx_win_backend);

    RECT win_rect = { 0, 0, width, height };
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

    #define X(ret, name, args) name = (void*)wglGetProcAddress(#name);
    #   include "opengl_funcs_xlist.h"
    #undef X

    // TODO: check functions for NULL?

    SetWindowLongPtrW(win->backend->window, GWLP_USERDATA, (LONG_PTR)win);

    ShowWindow(win->backend->window, SW_SHOW);
    glViewport(0, 0, win->width, win->height);

    return win;
}

void gfx_win_destroy(gfx_window* win) { 
    if (win == NULL) {
        return;
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(win->backend->context);
    ReleaseDC(win->backend->window, win->backend->dc);
    DestroyWindow(win->backend->window);
}

b32 gfx_win_make_current(gfx_window* win) { 
    if (win == NULL) {
        return false;
    }

    return wglMakeCurrent(win->backend->dc, win->backend->context);
}

void gfx_win_process_events(gfx_window* win) { 
    if (win == NULL) {
        return;
    }

    memcpy(win->prev_mouse_buttons, win->mouse_buttons, GFX_MB_COUNT);
    memcpy(win->prev_keys, win->keys, GFX_KEY_COUNT);
    win->mouse_scroll = 0;

    MSG msg = { 0 };
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void gfx_win_clear_color(gfx_window* win, vec4f col) { 
    if (win == NULL) {
        return;
    }


    glClearColor(col.x, col.y, col.z, col.w);
}

void gfx_win_clear(gfx_window* win) { 
    if (win == NULL) {
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT);
}

void gfx_win_swap_buffers(gfx_window* win) { 
    if (win == NULL) {
        return;
    }

    SwapBuffers(win->backend->dc);
}

static LRESULT CALLBACK _w32_window_proc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    gfx_window* win = (gfx_window*)GetWindowLongPtrW(wnd, GWLP_USERDATA);

    switch (msg) {
        case WM_MOUSEMOVE: {
            win->mouse_pos.x = (f32)((l_param) & 0xffff);
            win->mouse_pos.y = (f32)((l_param >> 16) & 0xffff);
        } break;

        case WM_LBUTTONDOWN: {
            win->mouse_buttons[GFX_MB_LEFT] = true;
        } break;
        case WM_LBUTTONUP: {
            win->mouse_buttons[GFX_MB_LEFT] = false;
        } break;
        case WM_MBUTTONDOWN: {
            win->mouse_buttons[GFX_MB_MIDDLE] = true;
        } break;
        case WM_MBUTTONUP: {
            win->mouse_buttons[GFX_MB_MIDDLE] = false;
        } break;
        case WM_RBUTTONDOWN: {
            win->mouse_buttons[GFX_MB_RIGHT] = true;
        } break;
        case WM_RBUTTONUP: {
            win->mouse_buttons[GFX_MB_RIGHT] = false;
        } break;

        case WM_MOUSEWHEEL: {
            win->mouse_scroll = SIGN(GET_WHEEL_DELTA_WPARAM(w_param));
        } break;

        case WM_KEYDOWN: {
            gfx_key down_key = _w32_keymap[w_param];
            win->keys[down_key] = true;
        } break;
        case WM_KEYUP: {
            gfx_key up_key = _w32_keymap[w_param];
            win->keys[up_key] = false;
        } break;

        case WM_SIZE: {
            u32 width = (u32)LOWORD(l_param);
            u32 height = (u32)HIWORD(l_param);

            win->width = width;
            win->height = height;

            glViewport(0, 0, width, height);
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
        (void*)wglGetProcAddress("wglGetExtensionsStringARB");

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
            wglChoosePixelFormatARB = (void*)wglGetProcAddress("wglChoosePixelFormatARB");
        } else if (str8_equals(cur_str, STR8_LIT("WGL_ARB_create_context"))) {
            wglCreateContextAttribsARB = (void*)wglGetProcAddress("wglCreateContextAttribsARB");
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
    _w32_keymap[VK_BACK] = GFX_KEY_BACKSPACE;
    _w32_keymap[VK_TAB] = GFX_KEY_TAB;
    _w32_keymap[VK_RETURN] = GFX_KEY_ENTER;
    _w32_keymap[VK_CAPITAL] = GFX_KEY_CAPSLOCK;
    _w32_keymap[VK_ESCAPE] = GFX_KEY_ESCAPE;
    _w32_keymap[VK_SPACE] = GFX_KEY_SPACE;
    _w32_keymap[VK_PRIOR] = GFX_KEY_PAGEUP;
    _w32_keymap[VK_NEXT] = GFX_KEY_PAGEDOWN;
    _w32_keymap[VK_END] = GFX_KEY_END;
    _w32_keymap[VK_HOME] = GFX_KEY_HOME;
    _w32_keymap[VK_LEFT] = GFX_KEY_LEFT;
    _w32_keymap[VK_UP] = GFX_KEY_UP;
    _w32_keymap[VK_RIGHT] = GFX_KEY_RIGHT;
    _w32_keymap[VK_DOWN] = GFX_KEY_DOWN;
    _w32_keymap[VK_INSERT] = GFX_KEY_INSERT;
    _w32_keymap[VK_DELETE] = GFX_KEY_DELETE;
    _w32_keymap[0x30] = GFX_KEY_0;
    _w32_keymap[0x31] = GFX_KEY_1;
    _w32_keymap[0x32] = GFX_KEY_2;
    _w32_keymap[0x33] = GFX_KEY_3;
    _w32_keymap[0x34] = GFX_KEY_4;
    _w32_keymap[0x35] = GFX_KEY_5;
    _w32_keymap[0x36] = GFX_KEY_6;
    _w32_keymap[0x37] = GFX_KEY_7;
    _w32_keymap[0x38] = GFX_KEY_8;
    _w32_keymap[0x39] = GFX_KEY_9;
    _w32_keymap[0x41] = GFX_KEY_A;
    _w32_keymap[0x42] = GFX_KEY_B;
    _w32_keymap[0x43] = GFX_KEY_C;
    _w32_keymap[0x44] = GFX_KEY_D;
    _w32_keymap[0x45] = GFX_KEY_E;
    _w32_keymap[0x46] = GFX_KEY_F;
    _w32_keymap[0x47] = GFX_KEY_G;
    _w32_keymap[0x48] = GFX_KEY_H;
    _w32_keymap[0x49] = GFX_KEY_I;
    _w32_keymap[0x4A] = GFX_KEY_J;
    _w32_keymap[0x4B] = GFX_KEY_K;
    _w32_keymap[0x4C] = GFX_KEY_L;
    _w32_keymap[0x4D] = GFX_KEY_M;
    _w32_keymap[0x4E] = GFX_KEY_N;
    _w32_keymap[0x4F] = GFX_KEY_O;
    _w32_keymap[0x50] = GFX_KEY_P;
    _w32_keymap[0x51] = GFX_KEY_Q;
    _w32_keymap[0x52] = GFX_KEY_R;
    _w32_keymap[0x53] = GFX_KEY_S;
    _w32_keymap[0x54] = GFX_KEY_T;
    _w32_keymap[0x55] = GFX_KEY_U;
    _w32_keymap[0x56] = GFX_KEY_V;
    _w32_keymap[0x57] = GFX_KEY_W;
    _w32_keymap[0x58] = GFX_KEY_X;
    _w32_keymap[0x59] = GFX_KEY_Y;
    _w32_keymap[0x5A] = GFX_KEY_Z;
    _w32_keymap[VK_NUMPAD0] = GFX_KEY_NUMPAD_0;
    _w32_keymap[VK_NUMPAD1] = GFX_KEY_NUMPAD_1;
    _w32_keymap[VK_NUMPAD2] = GFX_KEY_NUMPAD_2;
    _w32_keymap[VK_NUMPAD3] = GFX_KEY_NUMPAD_3;
    _w32_keymap[VK_NUMPAD4] = GFX_KEY_NUMPAD_4;
    _w32_keymap[VK_NUMPAD5] = GFX_KEY_NUMPAD_5;
    _w32_keymap[VK_NUMPAD6] = GFX_KEY_NUMPAD_6;
    _w32_keymap[VK_NUMPAD7] = GFX_KEY_NUMPAD_7;
    _w32_keymap[VK_NUMPAD8] = GFX_KEY_NUMPAD_8;
    _w32_keymap[VK_NUMPAD9] = GFX_KEY_NUMPAD_9;
    _w32_keymap[VK_MULTIPLY] = GFX_KEY_NUMPAD_MULTIPLY;
    _w32_keymap[VK_ADD] = GFX_KEY_NUMPAD_ADD;
    _w32_keymap[VK_SUBTRACT] = GFX_KEY_NUMPAD_SUBTRACT;
    _w32_keymap[VK_DECIMAL] = GFX_KEY_NUMPAD_DECIMAL;
    _w32_keymap[VK_DIVIDE] = GFX_KEY_NUMPAD_DIVIDE;
    _w32_keymap[VK_F1] = GFX_KEY_F1;
    _w32_keymap[VK_F2] = GFX_KEY_F2;
    _w32_keymap[VK_F3] = GFX_KEY_F3;
    _w32_keymap[VK_F4] = GFX_KEY_F4;
    _w32_keymap[VK_F5] = GFX_KEY_F5;
    _w32_keymap[VK_F6] = GFX_KEY_F6;
    _w32_keymap[VK_F7] = GFX_KEY_F7;
    _w32_keymap[VK_F8] = GFX_KEY_F8;
    _w32_keymap[VK_F9] = GFX_KEY_F9;
    _w32_keymap[VK_F10] = GFX_KEY_F10;
    _w32_keymap[VK_F11] = GFX_KEY_F11;
    _w32_keymap[VK_F12] = GFX_KEY_F12;
    _w32_keymap[VK_NUMLOCK] = GFX_KEY_NUM_LOCK;
    _w32_keymap[VK_SCROLL] = GFX_KEY_SCROLL_LOCK;
    _w32_keymap[VK_LSHIFT] = GFX_KEY_LSHIFT;
    _w32_keymap[VK_RSHIFT] = GFX_KEY_RSHIFT;
    _w32_keymap[VK_LCONTROL] = GFX_KEY_LCONTROL;
    _w32_keymap[VK_RCONTROL] = GFX_KEY_RCONTROL;
    _w32_keymap[VK_LMENU] = GFX_KEY_LALT;
    _w32_keymap[VK_RMENU] = GFX_KEY_RALT;
    _w32_keymap[VK_OEM_1] = GFX_KEY_SEMICOLON;
    _w32_keymap[VK_OEM_PLUS] = GFX_KEY_EQUAL;
    _w32_keymap[VK_OEM_COMMA] = GFX_KEY_COMMA;
    _w32_keymap[VK_OEM_MINUS] = GFX_KEY_MINUS;
    _w32_keymap[VK_OEM_PERIOD] = GFX_KEY_PERIOD;
    _w32_keymap[VK_OEM_2] = GFX_KEY_FORWARDSLASH;
    _w32_keymap[VK_OEM_3] = GFX_KEY_BACKTICK;
    _w32_keymap[VK_OEM_4] = GFX_KEY_LBRACKET;
    _w32_keymap[VK_OEM_5] = GFX_KEY_BACKSLASH;
    _w32_keymap[VK_OEM_6] = GFX_KEY_RBRACKET;
    _w32_keymap[VK_OEM_7] = GFX_KEY_APOSTROPHE;
}

static b32 _w32_gfx_init(void) {
    _w32_instance = GetModuleHandleW(NULL);

    if (_w32_instance == NULL) {
        error_emit("Failed to get module handle");
        return false;
    }

    b32 ret = true;

    ret *= _w32_get_wgl_funcs();
    ret *= _w32_register_class();
    _w32_init_keymap();

    return ret;
}

#endif // PLATFORM_WIN32
