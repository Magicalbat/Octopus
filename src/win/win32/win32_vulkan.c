
b32 _win_equip_gfx(mem_arena* arena, window* win) {
    if (!vk_state.initialized) {
        error_emit("Cannot equip window graphics: Vulkan is not initialized");
        return false;
    }

    mem_arena_temp maybe_temp = arena_temp_begin(arena);
    win->gfx_info = PUSH_STRUCT(maybe_temp.arena, _win_gfx_info);

    VkWin32SurfaceCreateInfoKHR surface_create_info = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = GetModuleHandleW(NULL),
        .hwnd = win->plat_info->window,
    };

    VkResult res = vkCreateWin32SurfaceKHR(
        vk_state.instance, &surface_create_info,
        NULL, &win->gfx_info->vk.surface
    );
    
    if (res != VK_SUCCESS) {
        error_emit("Failed to create win32 vulkan surface for window");
        goto error;
    }

    if (!_win_vk_equip_gfx(&win->gfx_info->vk)) {
        goto error;
    }

    return true;

error:
    _win_vk_unequip_gfx(&win->gfx_info->vk);
    arena_temp_end(maybe_temp);

    return false;
}

void _win_unequip_gfx(window* win) {
    if (win->gfx_info == NULL) { return; }

    _win_vk_unequip_gfx(&win->gfx_info->vk);
}

void win_make_current(window* win) {
}

void win_begin_frame(window* win) {
}

void win_end_frame(window* win) {
}

