
static void* _w32gl_get_proc(const char* name);

static b32 _w32gl_get_wgl_funcs(void);

b32 _win_create_graphics(mem_arena* arena, window* win) {
    if (
        wglChoosePixelFormatARB == NULL ||
        wglCreateContextAttribsARB == NULL
    ) {
        if (!_w32gl_get_wgl_funcs()) {
            return false;
        }
    }

    return true;
}

void _win_destroy_graphics(window* win) {
}

void win_set_clear_color(window* win, vec4f col);

void win_begin_frame(window* win);

void win_end_frame(window* win);

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

static b32 _w32gl_get_wgl_funcs(void) {
    const u16* dummy_class_name = L"OctopusDummyWindow";

    HINSTANCE module_handle =GetModuleHandleW(NULL); 

    WNDCLASSW dummy_wnd_class = {
        .lpfnWndProc = DefWindowProcW,
        .hInstance = module_handle,
        .lpszClassName = dummy_class_name
    };

    if (!RegisterClassW(&dummy_wnd_class)) {
        error_emit("Failed to register dummy window class for wgl");
        return false;
    }

    HWND dummy_win = CreateWindowW(
        dummy_class_name, L"", WS_OVERLAPPED,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, NULL, NULL
    );

    if (dummy_win == NULL) {
        error_emit("Failed to create dummy window for wgl");
        return false;
    }

    HDC device_context = GetDC(dummy_win);
    if (device_context == NULL) {
        error_emit("Failed to get device context for dummy window for wgl");
        return false;
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
        return false;
    }

    if (!DescribePixelFormat(
        device_context, format, sizeof(PIXELFORMATDESCRIPTOR), &pixel_desc
    )) {
        error_emit("Failed to describe pixel format for dummy window for wgl"); 
        return false;
    }

    if (!SetPixelFormat(device_context, format, &pixel_desc)) {
        error_emit("Failed to set pixel format for dummy window for wgl"); 
        return false;
    }

    HGLRC dummy_context = wglCreateContext(device_context);

    if (dummy_context == NULL) {
        error_emit("Failed to create OpenGL context for dummy window for wgl"); 
        return false;
    }

    if (!wglMakeCurrent(device_context, dummy_context)) {
        error_emit(
            "Failed to make OpenGL context current for dummy window for wgl"
        ); 
        return false;
    }

    wglChoosePixelFormatARB = (wglChoosePixelFormatARB_func*)_w32gl_get_proc("wglChoosePixelFormatARB");
    wglCreateContextAttribsARB = (wglCreateContextAttribsARB_func*)_w32gl_get_proc("wglCreateContextAttribsARB");

    if (
        wglChoosePixelFormatARB == NULL ||
        wglCreateContextAttribsARB == NULL
    ) {
        error_emit("Failed to load wgl extensions");
        return false;
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(dummy_context);
    ReleaseDC(dummy_win, device_context);
    DestroyWindow(dummy_win);
    UnregisterClassW(dummy_class_name, module_handle);

    return true;
}

