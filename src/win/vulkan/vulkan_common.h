
typedef struct {
    u32 family;
    u32 index;
    VkQueue queue;
} win_vk_queue;

// Global program level state
typedef struct {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    
    VkDevice device;
    
    win_vk_queue graphics_queue;
    win_vk_queue compute_queue;

    b8 initialized;
} win_vk_global_state;

// Per-window state
typedef struct {
    VkSurfaceKHR surface;
} win_vk_local_state;

win_vk_global_state vk_state = { 0 };

// The surface must be created before calling this function because surface 
// creation is platform-dependent
b32 _win_vk_equip_gfx(win_vk_local_state* win_vk);

void _win_vk_unequip_gfx(win_vk_local_state* win_vk);

