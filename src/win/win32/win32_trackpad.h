
#include <hidclass.h>
#include <hidsdi.h>
#include <hidpi.h>

typedef enum {
    _W32_TRACKPAD_GES_NONE = 0,
    _W32_TRACKPAD_GES_UNDECIDED,
    _W32_TRACKPAD_GES_PAN,
    _W32_TRACKPAD_GES_ZOOM
} _w32_trackpad_gesture_state;

typedef struct {
    u32 rawinput_size;

    HANDLE device_handle;
    PHIDP_PREPARSED_DATA ppd;
    RAWINPUT* rawinput;

    u64 prev_input_us;

    u32 max_link_collections;
    u32 num_link_collections;
    u16* link_collections;

    i16 x_min_logical;
    i16 x_max_logical;
    i16 y_min_logical;
    i16 y_max_logical;

    f32 x_min_mm;
    f32 x_max_mm;
    f32 y_min_mm;
    f32 y_max_mm;

    // Both in (logical units)^2
    f32 pan_sqr_deadzone;
    f32 zoom_sqr_deadzone;

    // Gesture start positions (tracking two contacts)
    v2_i16 ges_start0;
    v2_i16 ges_start1;

    // Previous position of gesture contacts
    v2_i16 ges_prev0;
    v2_i16 ges_prev1;

    b8 has_trackpad;
    b8 initialized;

    _w32_trackpad_gesture_state gesture_state;
} _w32_trackpad_context;

_w32_trackpad_context* _w32_trackpad_init(mem_arena* arena, HWND hwnd);

b32 _w32_trackpad_detect_zoom(
    _w32_trackpad_context* context,
    HWND hWnd, LPARAM lParam
);

void _w32_trackpad_update(_w32_trackpad_context* context);

