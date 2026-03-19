
static b32 _w32gl_backend_initialized = false;
static HGLRC _w32gl_context = NULL;
static i32 _w32gl_pixel_format = 0;

static void* _w32gl_get_proc(const char* name) {
    void* p = (void*)wglGetProcAddress(name);

    if (
        p == (void*)(1) || p == (void*)(2) ||
        p == (void*)(3) || p == (void*)(-1)
    ) {
        p = NULL;
    }

    return p;
}

void win_gfx_backend_init(void) {
    const u16* dummy_class_name = L"OctopusDummyWindow";

    HINSTANCE module_handle = GetModuleHandleW(NULL); 

    WNDCLASSW dummy_wnd_class = {
        .lpfnWndProc = DefWindowProcW,
        .hInstance = module_handle,
        .lpszClassName = dummy_class_name
    };

    if (!RegisterClassW(&dummy_wnd_class)) {
        plat_fatal_error("Failed to register dummy window class for wgl", 1);
    }

    HWND dummy_win = CreateWindowW(
        dummy_class_name, L"", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, NULL, NULL
    );

    if (dummy_win == NULL) {
        plat_fatal_error("Failed to create dummy window for wgl", 1);
    }

    HDC device_context = GetDC(dummy_win);
    if (device_context == NULL) {
        plat_fatal_error("Failed to get device context for dummy window for wgl", 1);
    }

    PIXELFORMATDESCRIPTOR pixel_desc = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 24,
    };

    i32 format = ChoosePixelFormat(device_context, &pixel_desc);

    if (format == 0) {
        plat_fatal_error("Failed to choose pixel format for dummy window for wgl", 1); 
    }

    if (!DescribePixelFormat(
        device_context, format, sizeof(PIXELFORMATDESCRIPTOR), &pixel_desc
    )) {
        plat_fatal_error("Failed to describe pixel format for dummy window for wgl", 1);
    }

    if (!SetPixelFormat(device_context, format, &pixel_desc)) {
        plat_fatal_error("Failed to set pixel format for dummy window for wgl", 1); 
    }

    HGLRC dummy_context = wglCreateContext(device_context);

    if (dummy_context == NULL) {
        plat_fatal_error("Failed to create OpenGL context for dummy window for wgl", 1); 
    }

    if (!wglMakeCurrent(device_context, dummy_context)) {
        plat_fatal_error("Failed to make OpenGL context current for dummy window for wgl", 1); 
    }

    wglChoosePixelFormatARB = (wglChoosePixelFormatARB_func*)_w32gl_get_proc("wglChoosePixelFormatARB");
    wglCreateContextAttribsARB = (wglCreateContextAttribsARB_func*)_w32gl_get_proc("wglCreateContextAttribsARB");
    wglSwapIntervalEXT = (wglSwapIntervalEXT_func*)_w32gl_get_proc("wglSwapIntervalEXT");

    if (
        wglChoosePixelFormatARB == NULL ||
        wglCreateContextAttribsARB == NULL ||
        wglSwapIntervalEXT == NULL
    ) {
        plat_fatal_error("Failed to load wgl extensions", 1);
    }

    i32 pf_attribs[] ={
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
        WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB,     32,
        WGL_DEPTH_BITS_ARB,     24,
        WGL_STENCIL_BITS_ARB,   8,
        0,
    }; 

    u32 num_formats = 0;
    if (!wglChoosePixelFormatARB(
        device_context, pf_attribs, NULL, 1, &_w32gl_pixel_format, &num_formats
    )) {
        plat_fatal_error("Failed to choose pixel format", 1);
    }

    // Enables VSync (I think)
    wglSwapIntervalEXT(1);

#define X(ret, name, args) \
    name = (gl_##name##_func*)_w32gl_get_proc(#name); \
    if (name == NULL) { plat_fatal_error("Failed to load OpenGL functions", 1); }
#   include "../opengl/opengl_funcs_xlist.h"
#undef X

    _w32gl_backend_initialized = true;

    wglMakeCurrent(device_context, NULL);
    wglDeleteContext(dummy_context);
    ReleaseDC(dummy_win, device_context);
    DestroyWindow(dummy_win);
    UnregisterClassW(dummy_class_name, module_handle);
}

b32 _win_equip_gfx(mem_arena* arena, window* win) {
    if (!_w32gl_backend_initialized) {
        error_emit("Win32 OpenGL backend is not initialized");
        return false; 
    }

    mem_arena_temp maybe_temp = arena_temp_begin(arena);

    win->gfx_info = PUSH_STRUCT(arena, _win_gfx_info);
    win->gfx_info->hdc = GetDC(win->plat_info->window);

    PIXELFORMATDESCRIPTOR pfd = { 0 };
    if (!DescribePixelFormat(win->gfx_info->hdc, _w32gl_pixel_format, sizeof(pfd), &pfd)) {
        error_emit("Failed to describe pixel format for window");
        goto fail;
    }

    if (!SetPixelFormat(win->gfx_info->hdc, _w32gl_pixel_format, &pfd)) {
        error_emit("Failed to set pixel format for window");
        goto fail;
    }

    // To create a context, you have to already have a DC with a compatible 
    // pixel format. As such, you would need to make a second temporary window
    // in win_gfx_backend_init just to grab the context, which is not something
    // I wanted to do. 
    if (_w32gl_context == NULL) {
        i32 context_attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 5,

#ifndef NDEBUG
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
            0,
        };

        _w32gl_context = wglCreateContextAttribsARB(
            win->gfx_info->hdc, NULL, context_attribs
        );

        if (_w32gl_context == NULL) {
            error_emit("Failed to create modern gl context");
            goto fail;
        }
    }

    return true;

fail:
    arena_temp_end(maybe_temp);
    return false;
}

void _win_unequip_gfx(window* win) {
    if (win != NULL && win->plat_info != NULL && win->gfx_info != NULL) {
        ReleaseDC(win->plat_info->window, win->gfx_info->hdc);
    }
}

void win_make_current(window* win) {
    if (win != NULL && win->gfx_info != NULL && _w32gl_backend_initialized) {
        wglMakeCurrent(win->gfx_info->hdc, _w32gl_context);
    }
}

void _win_gfx_swap(window* win) {
    if (win != NULL && win->gfx_info != NULL) {
        SwapBuffers(win->gfx_info->hdc);
    }
}

