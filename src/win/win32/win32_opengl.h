
#include <gl/GL.h>

typedef struct _win_api_backend {
    HDC device_context;
    HGLRC gl_context;
} _win_api_backend;

// WGL_ARB_pixel_format
// https://registry.khronos.org/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt
typedef BOOL (wglChoosePixelFormatARB_func)(
    HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList,
    UINT nMaxFormats, int *piFormats, UINT *nNumFormats
);
wglChoosePixelFormatARB_func* wglChoosePixelFormatARB = NULL;

#define WGL_DRAW_TO_WINDOW_ARB                    0x2001
#define WGL_SUPPORT_OPENGL_ARB                    0x2010
#define WGL_DOUBLE_BUFFER_ARB                     0x2011
#define WGL_PIXEL_TYPE_ARB                        0x2013
#define WGL_COLOR_BITS_ARB                        0x2014
#define WGL_DEPTH_BITS_ARB                        0x2022
#define WGL_STENCIL_BITS_ARB                      0x2023
#define WGL_TYPE_RGBA_ARB                         0x202B

// WGL_ARB_create_context
// https://registry.khronos.org/OpenGL/extensions/ARB/WGL_ARB_create_context.txt
typedef HGLRC (wglCreateContextAttribsARB_func)(HDC hDC, HGLRC hShareContext, const int *attribList);
wglCreateContextAttribsARB_func* wglCreateContextAttribsARB = NULL;

#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

