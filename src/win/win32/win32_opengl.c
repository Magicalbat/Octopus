
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
        error_emit("Failed to register dummy window class for wgl");

        goto end_before_class;
    }

    HWND dummy_win = CreateWindowW(
        dummy_class_name, L"", WS_OVERLAPPED,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, NULL, NULL
    );

    if (dummy_win == NULL) {
        error_emit("Failed to create dummy window for wgl");
        goto end_before_win;
    }

    HDC device_context = GetDC(dummy_win);
    if (device_context == NULL) {
        error_emit("Failed to get device context for dummy window for wgl");
        goto end_before_dc;
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
        error_emit("Failed to choose pixel format for dummy window for wgl"); 
        goto end_before_dummy_context;
    }

    if (!DescribePixelFormat(
        device_context, format, sizeof(PIXELFORMATDESCRIPTOR), &pixel_desc
    )) {
        error_emit("Failed to describe pixel format for dummy window for wgl"); 
        goto end_before_dummy_context;
    }

    if (!SetPixelFormat(device_context, format, &pixel_desc)) {
        error_emit("Failed to set pixel format for dummy window for wgl"); 
        goto end_before_dummy_context;
    }

    HGLRC dummy_context = wglCreateContext(device_context);

    if (dummy_context == NULL) {
        error_emit("Failed to create OpenGL context for dummy window for wgl"); 
        goto end_before_dummy_context;
    }

    if (!wglMakeCurrent(device_context, dummy_context)) {
        error_emit(
            "Failed to make OpenGL context current for dummy window for wgl"
        ); 
        goto end_before_dummy_make_current;
    }

    wglChoosePixelFormatARB = (wglChoosePixelFormatARB_func*)_w32gl_get_proc("wglChoosePixelFormatARB");
    wglCreateContextAttribsARB = (wglCreateContextAttribsARB_func*)_w32gl_get_proc("wglCreateContextAttribsARB");

    if (
        wglChoosePixelFormatARB == NULL ||
        wglCreateContextAttribsARB == NULL
    ) {
        error_emit("Failed to load wgl extensions");
        goto end;
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
        error_emit("Failed to create modern gl context");
        return false;
    }

end:
    wglMakeCurrent(NULL, NULL);
end_before_dummy_make_current:
    wglDeleteContext(dummy_context);
end_before_dummy_context:
    ReleaseDC(dummy_win, device_context);
end_before_dc:
    DestroyWindow(dummy_win);
end_before_win:
    UnregisterClassW(dummy_class_name, module_handle);
end_before_class:

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

