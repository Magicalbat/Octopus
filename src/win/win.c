
b32 _win_equip_gfx(mem_arena* arena, window* win);
void _win_unequip_gfx(window* win);
void _win_gfx_swap(window* win);

#if defined(PLATFORM_WIN32)
#   include "win32/win32_common.c"
#elif defined(PLATFORM_LINUX)
#endif

#if defined(WIN_GFX_API_OPENGL)
#   include "opengl/opengl_common.c"
#   if defined(PLATFORM_WIN32)
#       include "win32/win32_opengl.c"
#   elif defined(PLATFORM_LINUX)
#   endif
#endif


