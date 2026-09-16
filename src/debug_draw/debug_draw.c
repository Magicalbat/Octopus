
#if defined(WIN_GFX_API_OPENGL)
#   include "debug_draw_gl.c"
#elif defined(WIN_GFX_API_VULKAN)
#   include "debug_draw_vk.c"
#endif

