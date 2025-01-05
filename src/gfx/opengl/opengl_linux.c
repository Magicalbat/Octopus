#include "base/base_defs.h"

#ifdef PLATFORM_LINUX

#include "platform/platform.h"
#include "gfx/gfx.h"
#include "opengl_defs.h"

#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <GL/glx.h>
#include <GL/gl.h>

typedef struct _gfx_win_backend {
    Display* display;
    i32 screen;
    GLXFBConfig fb_config;
    Window window;
    GLXContext gl_context;
    Atom del_atom;
} _gfx_win_backend;

typedef GLXContext (*glXCreateContextAttribsARBProc) (Display*, GLXFBConfig, GLXContext, Bool, const int*);

#define X(ret, name, args) gl_##name##_func name = NULL;
#   include "opengl_funcs_xlist.h"
#undef X

static gfx_key x11_translate_key(XKeyEvent* e);

gfx_window* gfx_win_create(mem_arena* arena, u32 width, u32 height, string8 title) {
    gfx_window* win = ARENA_PUSH(arena, gfx_window);

    *win = (gfx_window){
        .title = title,
        .width = width,
        .height = height,
        .backend = ARENA_PUSH(arena, _gfx_win_backend)
    };

    win->backend->display = XOpenDisplay(NULL);
    if (win->backend->display == NULL) {
        plat_fatal_error("Failed to open X11 display");
        return NULL;
    }

    win->backend->screen = DefaultScreen(win->backend->display);
    i32 major_version = 0;
    i32 minor_version = 0;
    glXQueryVersion(win->backend->display, &major_version, &minor_version);
    if (major_version <= 0 || minor_version <= 1) {
        XCloseDisplay(win->backend->display);
        plat_fatal_error("Invalid GLX version");
        return NULL;
    }

    i32 fbc_attribs[] = {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_DOUBLEBUFFER, True,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 0,
        GLX_STENCIL_SIZE, 0,
        None
    };

    i32 fb_count = 0;
    GLXFBConfig* fbc = glXChooseFBConfig(win->backend->display, win->backend->screen, fbc_attribs, &fb_count);
    if (fbc == NULL) {
        XCloseDisplay(win->backend->display);
        plat_fatal_error("Failed to retrieve framebuffer");
        return NULL;
    }

    i32 best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 9999;
    for (i32 i = 0; i < fb_count; i++) {
        XVisualInfo* vi = glXGetVisualFromFBConfig(win->backend->display, fbc[i]);

        if (vi == NULL) {
            XFree(vi);
            continue;
        }

        i32 sample_bufs = 0, samples = 0;
        glXGetFBConfigAttrib(win->backend->display, fbc[i], GLX_SAMPLE_BUFFERS, &sample_bufs);
        glXGetFBConfigAttrib(win->backend->display, fbc[i], GLX_SAMPLES, &samples);

        if (best_fbc < 0 || (sample_bufs && samples > best_num_samp)) {
            best_fbc = i;
            best_num_samp = samples;
        }
        if (worst_fbc < 0 || (!sample_bufs || samples < worst_num_samp)) {
            worst_fbc = i;
            worst_num_samp = samples;
        }

        XFree(vi);
    }

    win->backend->fb_config = fbc[best_fbc];

    XFree(fbc);

    XVisualInfo* visual = glXGetVisualFromFBConfig(win->backend->display, win->backend->fb_config);
    if (visual == NULL) {
        XCloseDisplay(win->backend->display);
        plat_fatal_error("Cannot create visual window");
        
        return NULL;
    }

    XSetWindowAttributes window_attribs = {
        .border_pixel = BlackPixel(win->backend->display, win->backend->screen),
        .background_pixel = WhitePixel(win->backend->display, win->backend->screen),
        .override_redirect = True,
        .colormap = XCreateColormap(win->backend->display, RootWindow(win->backend->display, win->backend->screen), visual->visual, AllocNone),
        .event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask | KeyReleaseMask
    };

    win->backend->window = XCreateWindow(
        win->backend->display,
        RootWindow(win->backend->display, win->backend->screen),
        0, 0, width, height, 0,
        visual->depth, InputOutput, visual->visual,
        CWBackPixel | CWColormap | CWBorderPixel | CWEventMask,
        &window_attribs
    );

    win->backend->del_atom = XInternAtom(win->backend->display, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(win->backend->display, win->backend->window, &win->backend->del_atom, 1);

    XMapWindow(win->backend->display, win->backend->window);

    XFree(visual);

    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");

    if (!glXCreateContextAttribsARB) {
        plat_fatal_error("glXCreateContextAttribsARB() not found");
    }

    static int context_attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
        GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        None
    };
    win->backend->gl_context = glXCreateContextAttribsARB(win->backend->display, win->backend->fb_config, 0, true, context_attribs);

    XSync(win->backend->display, false);

    XFreeColormap(win->backend->display, window_attribs.colormap);

    glXMakeCurrent(win->backend->display, win->backend->window, win->backend->gl_context);
    
    glViewport(0, 0, width, height);

    #define X(ret, name, args) name = (gl_##name##_func)glXGetProcAddress((const GLubyte*)#name);
    #    include "opengl_funcs_xlist.h"
    #undef X

    mem_arena_temp scratch = arena_scratch_get(NULL, 0);
    u8* title_cstr = str8_to_cstr(scratch.arena, title);
    XStoreName(win->backend->display, win->backend->window, (char*)title_cstr);
    arena_scratch_release(scratch);

    return win;
}

void gfx_win_destroy(gfx_window* win) {
    glXMakeCurrent(win->backend->display, None, NULL);
    glXDestroyContext(win->backend->display, win->backend->gl_context);

    XDestroyWindow(win->backend->display, win->backend->window);
    XCloseDisplay(win->backend->display);
}

b32 gfx_win_make_current(gfx_window* win) {
    return glXMakeCurrent(win->backend->display, win->backend->window, win->backend->gl_context);
}

void gfx_win_process_events(gfx_window* win) {
    memcpy(win->prev_mouse_buttons, win->mouse_buttons, GFX_MB_COUNT);
    memcpy(win->prev_keys, win->keys, GFX_KEY_COUNT);

    win->mouse_scroll = 0;
    
    while (XPending(win->backend->display)) {
        XEvent e = { 0 };
        XNextEvent(win->backend->display, &e);

        switch(e.type) {
            case Expose: {
                glViewport(0, 0, e.xexpose.width, e.xexpose.height);
                win->width = e.xexpose.width;
                win->height = e.xexpose.height;
            } break;
            case ButtonPress: {
                if (e.xbutton.button == 4) {
                    // Scroll up
                    win->mouse_scroll = 1;
                } else if (e.xbutton.button == 5) {
                    // Scroll down
                    win->mouse_scroll = -1;
                } else {
                    win->mouse_buttons[e.xbutton.button - 1] = true;
                }
            } break;
            case ButtonRelease: {
                if (e.xbutton.button != 4 && e.xbutton.button != 5)
                    win->mouse_buttons[e.xbutton.button - 1] = false;
            } break;
            case MotionNotify: {
                win->mouse_pos.x = (f32)e.xmotion.x;
                win->mouse_pos.y = (f32)e.xmotion.y;
            } break;
            case KeyPress: {
                gfx_key keydown = x11_translate_key(&e.xkey);
                win->keys[keydown] = true;
            } break;
            case KeyRelease: {
                gfx_key keyup = x11_translate_key(&e.xkey);
                win->keys[keyup] = false;
            } break;
            case ClientMessage: {
                if ((i64)e.xclient.data.l[0] == (i64)win->backend->del_atom) {
                    win->should_close = true;
                }
            } break;
        }
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
    glXSwapBuffers(win->backend->display, win->backend->window);
}

// Adapted from sokol_app.h
// https://github.com/floooh/sokol/blob/master/sokol_app.h#L10175 
static gfx_key x11_translate_key(XKeyEvent* e) {
    KeySym keysym = XLookupKeysym(e, 0);

    switch (keysym) {
        case XK_Escape:         return GFX_KEY_ESCAPE;
        case XK_Tab:            return GFX_KEY_TAB;
        case XK_Shift_L:        return GFX_KEY_LSHIFT;
        case XK_Shift_R:        return GFX_KEY_RSHIFT;
        case XK_Control_L:      return GFX_KEY_LCONTROL;
        case XK_Control_R:      return GFX_KEY_RCONTROL;
        case XK_Meta_L:
        case XK_Alt_L:          return GFX_KEY_LALT;
        case XK_Mode_switch:    /* Mapped to Alt_R on many keyboards */
        case XK_ISO_Level3_Shift: /* AltGr on at least some machines */
        case XK_Meta_R:
        case XK_Alt_R:          return GFX_KEY_RALT;
        case XK_Num_Lock:       return GFX_KEY_NUM_LOCK;
        case XK_Caps_Lock:      return GFX_KEY_CAPSLOCK;
        case XK_Scroll_Lock:    return GFX_KEY_SCROLL_LOCK;
        case XK_Delete:         return GFX_KEY_DELETE;
        case XK_BackSpace:      return GFX_KEY_BACKSPACE;
        case XK_Return:         return GFX_KEY_ENTER;
        case XK_Home:           return GFX_KEY_HOME;
        case XK_End:            return GFX_KEY_END;
        case XK_Page_Up:        return GFX_KEY_PAGEUP;
        case XK_Page_Down:      return GFX_KEY_PAGEDOWN;
        case XK_Insert:         return GFX_KEY_INSERT;
        case XK_Left:           return GFX_KEY_LEFT;
        case XK_Right:          return GFX_KEY_RIGHT;
        case XK_Down:           return GFX_KEY_DOWN;
        case XK_Up:             return GFX_KEY_UP;
        case XK_F1:             return GFX_KEY_F1;
        case XK_F2:             return GFX_KEY_F2;
        case XK_F3:             return GFX_KEY_F3;
        case XK_F4:             return GFX_KEY_F4;
        case XK_F5:             return GFX_KEY_F5;
        case XK_F6:             return GFX_KEY_F6;
        case XK_F7:             return GFX_KEY_F7;
        case XK_F8:             return GFX_KEY_F8;
        case XK_F9:             return GFX_KEY_F9;
        case XK_F10:            return GFX_KEY_F10;
        case XK_F11:            return GFX_KEY_F11;
        case XK_F12:            return GFX_KEY_F12;

        case XK_KP_Divide:      return GFX_KEY_NUMPAD_DIVIDE;
        case XK_KP_Multiply:    return GFX_KEY_NUMPAD_MULTIPLY;
        case XK_KP_Subtract:    return GFX_KEY_NUMPAD_SUBTRACT;
        case XK_KP_Add:         return GFX_KEY_NUMPAD_ADD;

        case XK_KP_Insert:      return GFX_KEY_NUMPAD_0;
        case XK_KP_End:         return GFX_KEY_NUMPAD_1;
        case XK_KP_Down:        return GFX_KEY_NUMPAD_2;
        case XK_KP_Page_Down:   return GFX_KEY_NUMPAD_3;
        case XK_KP_Left:        return GFX_KEY_NUMPAD_4;
        case XK_KP_Begin:       return GFX_KEY_NUMPAD_5;
        case XK_KP_Right:       return GFX_KEY_NUMPAD_6;
        case XK_KP_Home:        return GFX_KEY_NUMPAD_7;
        case XK_KP_Up:          return GFX_KEY_NUMPAD_8;
        case XK_KP_Page_Up:     return GFX_KEY_NUMPAD_9;
        case XK_KP_Delete:      return GFX_KEY_NUMPAD_DECIMAL;
        case XK_KP_Enter:       return GFX_KEY_NUMPAD_ENTER;

        case XK_a:              return GFX_KEY_A;
        case XK_b:              return GFX_KEY_B;
        case XK_c:              return GFX_KEY_C;
        case XK_d:              return GFX_KEY_D;
        case XK_e:              return GFX_KEY_E;
        case XK_f:              return GFX_KEY_F;
        case XK_g:              return GFX_KEY_G;
        case XK_h:              return GFX_KEY_H;
        case XK_i:              return GFX_KEY_I;
        case XK_j:              return GFX_KEY_J;
        case XK_k:              return GFX_KEY_K;
        case XK_l:              return GFX_KEY_L;
        case XK_m:              return GFX_KEY_M;
        case XK_n:              return GFX_KEY_N;
        case XK_o:              return GFX_KEY_O;
        case XK_p:              return GFX_KEY_P;
        case XK_q:              return GFX_KEY_Q;
        case XK_r:              return GFX_KEY_R;
        case XK_s:              return GFX_KEY_S;
        case XK_t:              return GFX_KEY_T;
        case XK_u:              return GFX_KEY_U;
        case XK_v:              return GFX_KEY_V;
        case XK_w:              return GFX_KEY_W;
        case XK_x:              return GFX_KEY_X;
        case XK_y:              return GFX_KEY_Y;
        case XK_z:              return GFX_KEY_Z;
        case XK_1:              return GFX_KEY_1;
        case XK_2:              return GFX_KEY_2;
        case XK_3:              return GFX_KEY_3;
        case XK_4:              return GFX_KEY_4;
        case XK_5:              return GFX_KEY_5;
        case XK_6:              return GFX_KEY_6;
        case XK_7:              return GFX_KEY_7;
        case XK_8:              return GFX_KEY_8;
        case XK_9:              return GFX_KEY_9;
        case XK_0:              return GFX_KEY_0;
        case XK_space:          return GFX_KEY_SPACE;
        case XK_minus:          return GFX_KEY_MINUS;
        case XK_equal:          return GFX_KEY_EQUAL;
        case XK_bracketleft:    return GFX_KEY_LBRACKET;
        case XK_bracketright:   return GFX_KEY_RBRACKET;
        case XK_backslash:      return GFX_KEY_BACKSLASH;
        case XK_semicolon:      return GFX_KEY_SEMICOLON;
        case XK_apostrophe:     return GFX_KEY_APOSTROPHE;
        case XK_grave:          return GFX_KEY_BACKTICK;
        case XK_comma:          return GFX_KEY_COMMA;
        case XK_period:         return GFX_KEY_PERIOD;
        case XK_slash:          return GFX_KEY_FORWARDSLASH;
        default:                return GFX_KEY_NONE;
    }

    return GFX_KEY_NONE;
}


#endif // PLATFORM_LINUX

