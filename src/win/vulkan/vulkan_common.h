
typedef struct {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;

    b8 initialized;
} win_vk_state;

win_vk_state vk_state = { 0 };

