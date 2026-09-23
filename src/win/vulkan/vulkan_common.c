
b32 _vk_init_instance(void);
b32 _vk_pick_physical_device(void);
b32 _vk_create_device(void);

// Preferred layers
const char* _vk_layers[] = {
#ifndef NDEBUG
    "VK_LAYER_KHRONOS_validation",
#endif
};

// Required instance extension
const char* _vk_extensions[] = {
    "VK_KHR_surface",

#if defined(PLATFORM_WIN32)
    "VK_KHR_win32_surface",
#endif
};

// Required device extensions
const char* _vk_device_extensions[] = {
    "VK_KHR_swapchain",
};

void win_gfx_backend_init(void) {
    if (vk_state.initialized) { return; }

    if (!_vk_init_instance()) { goto error; }
    if (!_vk_pick_physical_device()) { goto error; }
    if (!_vk_create_device()) { goto error; }

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

b32 _vk_init_instance(void) {
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

                return false;
            }
        } 

        arena_scratch_release(scratch);
    }

    // Creating instance
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
        return false;
    }

    return true;
}

b32 _vk_can_queue_present(VkPhysicalDevice physical_device, u32 queue_family) {
#if defined(PLATFORM_WIN32)
    return (b32)vkGetPhysicalDeviceWin32PresentationSupportKHR(
        physical_device, queue_family
    );
#endif

// Note(Ian) I think that the wayland and x11 versions of this function 
// require a reference to the display or something, so this may need to be
// reworked in the future
}

b32 _vk_physical_device_suitable(VkPhysicalDevice physical_device) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physical_device, &props);

    if (props.apiVersion < VK_API_VERSION_1_3) {
        return false;
    }

    // Checking for queue support
    {
        // Transfer queue implied
        b8 has_graphics_queue = false, has_compute_queue = false;
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);

        u32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(
            physical_device, &queue_family_count, NULL
        );

        VkQueueFamilyProperties* queue_family_props = PUSH_ARRAY(
            scratch.arena, VkQueueFamilyProperties, queue_family_count
        );
        vkGetPhysicalDeviceQueueFamilyProperties(
            physical_device, &queue_family_count, queue_family_props
        );

        for (u32 i = 0; i < queue_family_count; i++) {
            if (queue_family_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                has_compute_queue = true;
            }

            if (
                (queue_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                _vk_can_queue_present(physical_device, i)
            ) {
                has_graphics_queue = true;
            }
        }

        arena_scratch_release(scratch);

        if (!has_compute_queue || !has_graphics_queue) {
            return false;
        }
    }

    // Checking for extension support
    {
        mem_arena_temp scratch = arena_scratch_get(NULL, 0);

        u32 extension_props_count = 0;
        vkEnumerateDeviceExtensionProperties(
            physical_device, NULL, &extension_props_count, NULL
        );

        VkExtensionProperties* extension_props = PUSH_ARRAY(
            scratch.arena, VkExtensionProperties, extension_props_count
        );
        vkEnumerateDeviceExtensionProperties(
            physical_device, NULL, &extension_props_count, extension_props
        );

        b8 has_all_extensions = true;
        for (u32 i = 0; i < ARRAY_LEN(_vk_device_extensions); i++) {
            b8 extension_found = false;

            for (u32 j = 0; j < extension_props_count; j++) {
                if (strcmp(
                    _vk_device_extensions[i], extension_props[j].extensionName
                ) == 0) {
                    extension_found = true;
                    break;
                }
            }

            if (!extension_found) {
                has_all_extensions = false;
                break;
            }
        }

        arena_scratch_release(scratch);

        if (!has_all_extensions) {
            return false;
        }
    }

    // Note(Ian) Should we be checking feature support or does the 1.3+ check
    // cover everything needed?

    return true;
}

b32 _vk_pick_physical_device(void) {
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    u32 physical_device_count = 0;
    vkEnumeratePhysicalDevices(vk_state.instance, &physical_device_count, NULL);

    VkPhysicalDevice* physical_devices = PUSH_ARRAY(
        scratch.arena, VkPhysicalDevice, physical_device_count
    );
    vkEnumeratePhysicalDevices(
        vk_state.instance, &physical_device_count, physical_devices
    );

    u32 best_device_score = 0;
    VkPhysicalDevice best_physical_device = NULL;

    for (u32 i = 0; i < physical_device_count; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physical_devices[i], &props);

        if (!_vk_physical_device_suitable(physical_devices[i])) {
            continue; 
        }

        u32 score = 0;
        switch (props.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: {
                score += 1000;
            } break;

            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: {
                score += 2000;
            } break;

            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: {
                score += 3000;
            } break;

            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
            case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM: {
                score += 0;
            } break;
        }

        if (score > best_device_score) {
            best_device_score = score;
            best_physical_device = physical_devices[i];
        }
    }

    arena_scratch_release(scratch);

    if (best_physical_device == NULL) {
        error_emit("Failed to find suitable physical device for Vulkan");
        return false;
    }

    vk_state.physical_device = best_physical_device;

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(vk_state.physical_device, &props);
    info_emitf("Using device '%s' for Vulkan", props.deviceName);

    return true;
}

b32 _vk_create_device(void) {
    mem_arena_temp scratch = arena_scratch_get(NULL, 0);

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        vk_state.physical_device, &queue_family_count, NULL
    );

    VkQueueFamilyProperties* family_props = PUSH_ARRAY(
        scratch.arena, VkQueueFamilyProperties, queue_family_count
    );
    vkGetPhysicalDeviceQueueFamilyProperties(
        vk_state.physical_device, &queue_family_count, family_props
    );

    u32 graphics_family = UINT32_MAX, compute_family = UINT32_MAX;

    // Finding graphics family
    for (u32 i = 0; i < queue_family_count; i++) {
        if (
            (family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            _vk_can_queue_present(vk_state.physical_device, i)
        ) {
            graphics_family = i;
            break;
        }
    }

    if (graphics_family == UINT32_MAX) {
        error_emit("Unable to find graphics queue family for Vulkan");
        goto error;
    }

    // Finding compute family
    for (u32 i = 0; i < queue_family_count; i++) {
        if (
            (family_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
            (i != graphics_family || family_props[i].queueCount >= 2)
        ) {
            compute_family = i;
            break;
        }
    }

    if (compute_family == UINT32_MAX) {
        error_emit("Unable to find compute queue family for Vulkan");
        goto error;
    }

    u32 num_families = graphics_family == compute_family ? 1 : 2;
    VkDeviceQueueCreateInfo queue_create_infos[2] = { 0 };
    f32 queue_priorities[2] = { 0.5f, 0.5f };

    if (num_families == 1) {
        queue_create_infos[0] = (VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = graphics_family,
            .queueCount = 2,
            .pQueuePriorities = queue_priorities,
        };
    } else {
        queue_create_infos[0] = (VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = graphics_family,
            .queueCount = 1,
            .pQueuePriorities = queue_priorities,
        };

        queue_create_infos[1] = (VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = compute_family,
            .queueCount = 1,
            .pQueuePriorities = queue_priorities,
        };
    }

    VkPhysicalDeviceVulkan13Features device_13_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .dynamicRendering = true
    };

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &device_13_features,
        .queueCreateInfoCount = num_families,
        .pQueueCreateInfos = queue_create_infos,
        .enabledExtensionCount = ARRAY_LEN(_vk_device_extensions),
        .ppEnabledExtensionNames = _vk_device_extensions,
    };

    VkResult res = vkCreateDevice(
        vk_state.physical_device, &device_create_info,
        NULL, &vk_state.device
    );

    if (res != VK_SUCCESS) {
        error_emit("Failed to create Vulkan device");
        goto error;
    }

    vk_state.graphics_queue = (win_vk_queue) {
        .family = graphics_family,
        .index = 0,
    };
    
    vk_state.compute_queue = (win_vk_queue) {
        .family = compute_family,
        .index = graphics_family == compute_family ? 1 : 0,
    };

    vkGetDeviceQueue(
        vk_state.device,
        vk_state.graphics_queue.family,
        vk_state.graphics_queue.index,
        &vk_state.graphics_queue.queue
    );

    vkGetDeviceQueue(
        vk_state.device,
        vk_state.compute_queue.family,
        vk_state.compute_queue.index,
        &vk_state.compute_queue.queue
    );

    arena_scratch_release(scratch);

    return true;

error:
    arena_scratch_release(scratch);
    return false;
}
