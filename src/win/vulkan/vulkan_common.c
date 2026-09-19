
const char* _vk_layers[] = {
#ifndef NDEBUG
    "VK_LAYER_KHRONOS_validation",
#endif
};

const char* _vk_extensions[] = {
    "VK_KHR_surface",

#if defined(PLATFORM_WIN32)
    "VK_KHR_win32_surface",
#endif
};

void win_gfx_backend_init(void) {
    if (vk_state.initialized) { return; }

    // Finding the desired layers
    u32 enabled_layer_count = 0;
    const char* enabled_layers[ARRAY_LEN(_vk_layers)] = { 0 };
    {
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);

        u32 layer_count = 0;
        vkEnumerateInstanceLayerProperties(&layer_count, 0);

        VkLayerProperties* layer_props = PUSH_ARRAY(
            scratch.arena, VkLayerProperties, layer_count
        );
        vkEnumerateInstanceLayerProperties(&layer_count, layer_props);

        for (u32 i = 0; i < ARRAY_LEN(_vk_layers); i++) {
            b32 layer_found = false;

            for (u32 j = 0; j < layer_count; j++) {
                if (strcmp(_vk_layers[i], layer_props[j].layerName) == 0) {
                    layer_found = true;
                    break;
                }
            }

            if (layer_found) {
                enabled_layers[enabled_layer_count++] = _vk_layers[i];
            } else {
                warn_emitf("Unable to find Vulkan layer '%s'", _vk_layers[i]);
            }
        }

        arena_scratch_release(scratch);
    }

    // Checking extensions
    {
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);

        u32 extension_count = 0;
        vkEnumerateInstanceExtensionProperties(NULL, &extension_count, NULL);

        VkExtensionProperties* extension_props = PUSH_ARRAY(
            scratch.arena, VkExtensionProperties, extension_count
        );
        vkEnumerateInstanceExtensionProperties(
            NULL, &extension_count, extension_props
        );

        for (u32 i = 0; i < ARRAY_LEN(_vk_extensions); i++) {
            b32 layer_found = false;

            for (u32 j = 0; j < extension_count; j++) {
                if (strcmp(
                    _vk_extensions[i], extension_props[j].extensionName
                ) == 0) {
                    layer_found = true;
                    break;
                }
            }

            if (!layer_found) {
                error_emitf(
                    "Unable to find required Vulkan extension '%s'",
                    _vk_extensions[i]
                );

                goto error;
            }
        } 

        arena_scratch_release(scratch);
    }

    // Creating instance
    {
        VkInstanceCreateInfo instance_create_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,

            .pApplicationInfo = &(VkApplicationInfo){
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pApplicationName = "Vulkan Learning",
                .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
                .pEngineName = "No Engine",
                .engineVersion = 0,
                .apiVersion = VK_API_VERSION_1_3,
            },

            .enabledLayerCount = enabled_layer_count,
            .ppEnabledLayerNames = enabled_layers,

            .enabledExtensionCount = ARRAY_LEN(_vk_extensions),
            .ppEnabledExtensionNames = _vk_extensions,
        };

        VkResult res = vkCreateInstance(
            &instance_create_info, NULL, &vk_state.instance
        );

        if (res != VK_SUCCESS) {
            error_emit("Unable to create Vulkan instance");
            goto error;
        }
    }

    vk_state.initialized = true;
    return;

error:
    win_gfx_backend_terminate();
}

void win_gfx_backend_terminate(void) {
    if (vk_state.device) { vkDestroyDevice(vk_state.device, NULL); }
    if (vk_state.instance) { vkDestroyInstance(vk_state.instance, NULL); }

    vk_state = (win_vk_state){ 0 };
}
