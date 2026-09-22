
typedef struct {
    u32 family;
    u32 index;
    VkQueue queue;
} win_vk_queue;

typedef struct {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    
    VkDevice device;
    
    win_vk_queue graphics_queue;
    win_vk_queue compute_queue;
    win_vk_queue transfer_queue;

    b8 initialized;
} win_vk_state;

win_vk_state vk_state = { 0 };

