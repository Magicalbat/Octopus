#include "base/base_defs.h"

#ifdef PLATFORM_WIN32

#include "gfx/gfx.h"
#include "platform/platform.h"
#include "opengl_defs.h"

#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <GL/gl.h>

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

static u32 w32_keymap[256] = { 0 };

static wglChoosePixelFormatARB_func* wglChoosePixelFormatARB = NULL;
static wglCreateContextAttribsARB_func* wglCreateContextAttribsARB = NULL;

static HINSTANCE w32_instance = NULL;
static const u16* w32_window_class_name = L"OctopusWindowClass";
static WNDCLASSEXW w32_window_class = { 0 };

static b32 w32_first_win = true;

static LRESULT CALLBACK w32_window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static void w32_get_wgl_funcs(void);
static void w32_register_class(void);
static void w32_init_keymap(void);
static void w32_gfx_init(void);

gfx_window* gfx_win_create(mem_arena* arena, u32 width, u32 height, string8 title) {
    if (w32_first_win) {
        w32_first_win = false;

        w32_gfx_init();
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
        w32_window_class_name,
        title16.str,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        win_rect.right - win_rect.left, win_rect.bottom - win_rect.top,
        NULL, NULL, w32_instance, NULL
    );

    arena_scratch_release(scratch);

    if (win->backend->window == NULL) {
        plat_fatal_error("Failed to create window");
    }

    win->backend->dc = GetDC(win->backend->window);

    if (win->backend->dc == NULL) {
        plat_fatal_error("Failed to get the device context for the window");
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
            plat_fatal_error("Failed to get required OpenGL pixel format");
        }

        PIXELFORMATDESCRIPTOR pixel_desc = { .nSize = sizeof(PIXELFORMATDESCRIPTOR) };
        if (!DescribePixelFormat(win->backend->dc, format, sizeof(PIXELFORMATDESCRIPTOR), &pixel_desc)) {
            plat_fatal_error("Failed to describe OpenGL pixel format");
        }

        if (!SetPixelFormat(win->backend->dc, format, &pixel_desc)) {
            plat_fatal_error("Failed to set OpenGL pixel format");
        }
    }

#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_FLAGS_ARB                     0x2094
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x0002

    {
        i32 attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 5,
            WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
            // Debug flag?
            0,
        };

        win->backend->context = wglCreateContextAttribsARB(win->backend->dc, NULL, attribs);

        if (win->backend->context == NULL) {
            plat_fatal_error("Failed to create modern OpenGL context (OpenGL 4.5 may not be supported)");
        }
    }

    if (!wglMakeCurrent(win->backend->dc, win->backend->context)) {
        plat_fatal_error("Failed to make OpenGL context current");
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
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(win->backend->context);
    ReleaseDC(win->backend->window, win->backend->dc);
    DestroyWindow(win->backend->window);
}

b32 gfx_win_make_current(gfx_window* win) { 
    return wglMakeCurrent(win->backend->dc, win->backend->context);
}

void gfx_win_process_events(gfx_window* win) { 
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
    UNUSED(win);

    glClearColor(col.x, col.y, col.z, col.w);
}

void gfx_win_clear(gfx_window* win) { 
    UNUSED(win);

    glClear(GL_COLOR_BUFFER_BIT);
}

void gfx_win_swap_buffers(gfx_window* win) { 
    SwapBuffers(win->backend->dc);
}

static LRESULT CALLBACK w32_window_proc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param) {
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
            gfx_key down_key = w32_keymap[w_param];
            win->keys[down_key] = true;
        } break;
        case WM_KEYUP: {
            gfx_key up_key = w32_keymap[w_param];
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

static void w32_get_wgl_funcs(void) {
    HWND dummy_window = CreateWindowW(
        L"STATIC", L"Dummy Window", WS_OVERLAPPED,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, NULL, NULL
    );

    if (dummy_window == NULL) {
        plat_fatal_error("Failed to create dummy window for WGL functions");
    }

    HDC dc = GetDC(dummy_window);

    if (dc == NULL) {
        plat_fatal_error("Failed to get device context for dummy window for WGL functions");
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
        plat_fatal_error("Failed to choose pixel format for dummy window for WGL functions");
    }

    if (!DescribePixelFormat(dc, format, sizeof(PIXELFORMATDESCRIPTOR), &pixel_desc)) {
        plat_fatal_error("Failed to describe pixel format for dummy window for WGL functions");
    }

    if (!SetPixelFormat(dc, format, &pixel_desc)) {
        plat_fatal_error("Failed to set pixel format for dummy window for WGL functions");
    }

    HGLRC context = wglCreateContext(dc);

    if (context == NULL) {
        plat_fatal_error("Failed to create OpenGL context for dummy window for WGL functions");
    }

    if (!wglMakeCurrent(dc, context)) {
        plat_fatal_error("Failed to make OpenGL context current for dummy window for WGL functions");
    }

    wglGetExtensionsStringARB_func* wglGetExtensionsStringARB =
        (void*)wglGetProcAddress("wglGetExtensionsStringARB");

    if (wglGetExtensionsStringARB == NULL) {
        plat_fatal_error("OpenGL does not support WGL_ARB_extensions_string extension");
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
        plat_fatal_error("WGL extensions required for modern contexts are not supported");
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(context);
    ReleaseDC(dummy_window, dc);
    DestroyWindow(dummy_window);
}

static void w32_register_class(void) {
    w32_window_class = (WNDCLASSEXW) {
        .cbSize = sizeof(WNDCLASSEXW),
        .lpfnWndProc = w32_window_proc,
        .hInstance = w32_instance,
        .hCursor = LoadCursorW(NULL, IDC_ARROW),
        .lpszClassName = w32_window_class_name
    };

    ATOM atom = RegisterClassExW(&w32_window_class);

    if (!atom) {
        plat_fatal_error("Failed to register window class");
    }
}

static void w32_init_keymap(void) {
    w32_keymap[VK_BACK] = GFX_KEY_BACKSPACE;
    w32_keymap[VK_TAB] = GFX_KEY_TAB;
    w32_keymap[VK_RETURN] = GFX_KEY_ENTER;
    w32_keymap[VK_CAPITAL] = GFX_KEY_CAPSLOCK;
    w32_keymap[VK_ESCAPE] = GFX_KEY_ESCAPE;
    w32_keymap[VK_SPACE] = GFX_KEY_SPACE;
    w32_keymap[VK_PRIOR] = GFX_KEY_PAGEUP;
    w32_keymap[VK_NEXT] = GFX_KEY_PAGEDOWN;
    w32_keymap[VK_END] = GFX_KEY_END;
    w32_keymap[VK_HOME] = GFX_KEY_HOME;
    w32_keymap[VK_LEFT] = GFX_KEY_LEFT;
    w32_keymap[VK_UP] = GFX_KEY_UP;
    w32_keymap[VK_RIGHT] = GFX_KEY_RIGHT;
    w32_keymap[VK_DOWN] = GFX_KEY_DOWN;
    w32_keymap[VK_INSERT] = GFX_KEY_INSERT;
    w32_keymap[VK_DELETE] = GFX_KEY_DELETE;
    w32_keymap[0x30] = GFX_KEY_0;
    w32_keymap[0x31] = GFX_KEY_1;
    w32_keymap[0x32] = GFX_KEY_2;
    w32_keymap[0x33] = GFX_KEY_3;
    w32_keymap[0x34] = GFX_KEY_4;
    w32_keymap[0x35] = GFX_KEY_5;
    w32_keymap[0x36] = GFX_KEY_6;
    w32_keymap[0x37] = GFX_KEY_7;
    w32_keymap[0x38] = GFX_KEY_8;
    w32_keymap[0x39] = GFX_KEY_9;
    w32_keymap[0x41] = GFX_KEY_A;
    w32_keymap[0x42] = GFX_KEY_B;
    w32_keymap[0x43] = GFX_KEY_C;
    w32_keymap[0x44] = GFX_KEY_D;
    w32_keymap[0x45] = GFX_KEY_E;
    w32_keymap[0x46] = GFX_KEY_F;
    w32_keymap[0x47] = GFX_KEY_G;
    w32_keymap[0x48] = GFX_KEY_H;
    w32_keymap[0x49] = GFX_KEY_I;
    w32_keymap[0x4A] = GFX_KEY_J;
    w32_keymap[0x4B] = GFX_KEY_K;
    w32_keymap[0x4C] = GFX_KEY_L;
    w32_keymap[0x4D] = GFX_KEY_M;
    w32_keymap[0x4E] = GFX_KEY_N;
    w32_keymap[0x4F] = GFX_KEY_O;
    w32_keymap[0x50] = GFX_KEY_P;
    w32_keymap[0x51] = GFX_KEY_Q;
    w32_keymap[0x52] = GFX_KEY_R;
    w32_keymap[0x53] = GFX_KEY_S;
    w32_keymap[0x54] = GFX_KEY_T;
    w32_keymap[0x55] = GFX_KEY_U;
    w32_keymap[0x56] = GFX_KEY_V;
    w32_keymap[0x57] = GFX_KEY_W;
    w32_keymap[0x58] = GFX_KEY_X;
    w32_keymap[0x59] = GFX_KEY_Y;
    w32_keymap[0x5A] = GFX_KEY_Z;
    w32_keymap[VK_NUMPAD0] = GFX_KEY_NUMPAD_0;
    w32_keymap[VK_NUMPAD1] = GFX_KEY_NUMPAD_1;
    w32_keymap[VK_NUMPAD2] = GFX_KEY_NUMPAD_2;
    w32_keymap[VK_NUMPAD3] = GFX_KEY_NUMPAD_3;
    w32_keymap[VK_NUMPAD4] = GFX_KEY_NUMPAD_4;
    w32_keymap[VK_NUMPAD5] = GFX_KEY_NUMPAD_5;
    w32_keymap[VK_NUMPAD6] = GFX_KEY_NUMPAD_6;
    w32_keymap[VK_NUMPAD7] = GFX_KEY_NUMPAD_7;
    w32_keymap[VK_NUMPAD8] = GFX_KEY_NUMPAD_8;
    w32_keymap[VK_NUMPAD9] = GFX_KEY_NUMPAD_9;
    w32_keymap[VK_MULTIPLY] = GFX_KEY_NUMPAD_MULTIPLY;
    w32_keymap[VK_ADD] = GFX_KEY_NUMPAD_ADD;
    w32_keymap[VK_SUBTRACT] = GFX_KEY_NUMPAD_SUBTRACT;
    w32_keymap[VK_DECIMAL] = GFX_KEY_NUMPAD_DECIMAL;
    w32_keymap[VK_DIVIDE] = GFX_KEY_NUMPAD_DIVIDE;
    w32_keymap[VK_F1] = GFX_KEY_F1;
    w32_keymap[VK_F2] = GFX_KEY_F2;
    w32_keymap[VK_F3] = GFX_KEY_F3;
    w32_keymap[VK_F4] = GFX_KEY_F4;
    w32_keymap[VK_F5] = GFX_KEY_F5;
    w32_keymap[VK_F6] = GFX_KEY_F6;
    w32_keymap[VK_F7] = GFX_KEY_F7;
    w32_keymap[VK_F8] = GFX_KEY_F8;
    w32_keymap[VK_F9] = GFX_KEY_F9;
    w32_keymap[VK_F10] = GFX_KEY_F10;
    w32_keymap[VK_F11] = GFX_KEY_F11;
    w32_keymap[VK_F12] = GFX_KEY_F12;
    w32_keymap[VK_NUMLOCK] = GFX_KEY_NUM_LOCK;
    w32_keymap[VK_SCROLL] = GFX_KEY_SCROLL_LOCK;
    w32_keymap[VK_LSHIFT] = GFX_KEY_LSHIFT;
    w32_keymap[VK_RSHIFT] = GFX_KEY_RSHIFT;
    w32_keymap[VK_LCONTROL] = GFX_KEY_LCONTROL;
    w32_keymap[VK_RCONTROL] = GFX_KEY_RCONTROL;
    w32_keymap[VK_LMENU] = GFX_KEY_LALT;
    w32_keymap[VK_RMENU] = GFX_KEY_RALT;
    w32_keymap[VK_OEM_1] = GFX_KEY_SEMICOLON;
    w32_keymap[VK_OEM_PLUS] = GFX_KEY_EQUAL;
    w32_keymap[VK_OEM_COMMA] = GFX_KEY_COMMA;
    w32_keymap[VK_OEM_MINUS] = GFX_KEY_MINUS;
    w32_keymap[VK_OEM_PERIOD] = GFX_KEY_PERIOD;
    w32_keymap[VK_OEM_2] = GFX_KEY_FORWARDSLASH;
    w32_keymap[VK_OEM_3] = GFX_KEY_BACKTICK;
    w32_keymap[VK_OEM_4] = GFX_KEY_LBRACKET;
    w32_keymap[VK_OEM_5] = GFX_KEY_BACKSLASH;
    w32_keymap[VK_OEM_6] = GFX_KEY_RBRACKET;
    w32_keymap[VK_OEM_7] = GFX_KEY_APOSTROPHE;
}

static void w32_gfx_init(void) {
    w32_instance = GetModuleHandleW(NULL);

    if (w32_instance == NULL) {
        plat_fatal_error("Failed to get module handle");
    }

    w32_get_wgl_funcs();
    w32_register_class();
    w32_init_keymap();
}

#endif // PLATFORM_WIN32
