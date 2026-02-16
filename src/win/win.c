
b32 _win_create_graphics(mem_arena* arena, window* win);
void _win_destroy_graphics(window* win);

#if defined(PLATFORM_WIN32)
#   include "win32/win32_common.c"
#elif defined(PLATFORM_LINUX)
#endif

#if defined(WIN_GFX_API_OPENGL)
#   if defined(PLATFORM_WIN32)
#       include "win32/win32_opengl.c"
#   elif defined(PLATFORM_LINUX)
#   endif
#endif


