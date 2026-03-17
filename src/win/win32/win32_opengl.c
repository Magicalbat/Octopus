
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

b32 _w32gl_init_backend(void) {
    b32 ret = true;

    const u16* dummy_class_name = L"OctopusDummyWindow";

    HINSTANCE module_handle =GetModuleHandleW(NULL); 

    WNDCLASSW dummy_wnd_class = {
        .lpfnWndProc = DefWindowProcW,
        .hInstance = module_handle,
        .lpszClassName = dummy_class_name
    };

    if (!RegisterClassW(&dummy_wnd_class)) {
        plat_fatal_error("Failed to register dummy window class for wgl", 1);
    }

    HWND dummy_win = CreateWindowW(
        dummy_class_name, L"", WS_OVERLAPPED,
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

    if (
        wglChoosePixelFormatARB == NULL ||
        wglCreateContextAttribsARB == NULL
    ) {
        plat_fatal_error("Failed to load wgl extensions", 1);
    }

    i32 context_attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 5,
        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,

#ifndef NDEBUG
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
        0,
    };

    _w32gl_context = wglCreateContextAttribsARB(
        device_context, dummy_context, context_attribs
    );

    if (_w32gl_context == NULL) {
        plat_fatal_error("Failed to create modern gl context", 1);
        return false;
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(dummy_context);
    ReleaseDC(dummy_win, device_context);
    DestroyWindow(dummy_win);
    UnregisterClassW(dummy_class_name, module_handle);

    return ret;
}

b32 _win_equip_gfx(window* win) {
    if (!_w32gl_backend_initialized) {
        error_emit("Win32 OpenGL backend is not initialized");
        return false; 
    }

    return true;
}

b32 _win_unequip_gfx(window* win);

void win_make_current(window* win);

void win_set_clear_color(window* win, vec4f col);

void win_begin_frame(window* win);

void win_end_frame(window* win);

