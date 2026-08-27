
#include <hidclass.h>
#include <hidsdi.h>
#include <hidpi.h>

#define _W32_TRACKPAD_MAX_CONTACTS 10

#define _W32_TRACKPAD_EXIT_TIMEOUT_US 16000
#define _W32_TRACKPAD_PAN_DEADZONE_MM 2
#define _W32_TRACKPAD_ZOOM_DEADZONE_MM 9

typedef enum {
    _W32_TRACKPAD_GES_NONE = 0,
    _W32_TRACKPAD_GES_UNDECIDED,
    _W32_TRACKPAD_GES_PAN,
    _W32_TRACKPAD_GES_ZOOM
} _w32_trackpad_gesture_state;

typedef struct {
    // USB HID Usage Page 0x01, Usage 0x30
    i16 x;

    // USB HID Usage Page 0x01, Usage 0x31
    i16 y;

    // USB HID Usage Page 0x0d, Usage 0x51
    i16 id;

    // USB HID Usage Page 0x0d, Usage 0x56
    u16 scan_time;
} _w32_trackpad_contact;


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

    // -1 indicates the first gesture scantime
    i32 prev_ges_scantime;

    // According to HUTRR83 (https://www.usb.org/sites/default/files/hutrr83_-_new_digitizer_usages_for_touchpads_0.pdf)
    // this value should reflect the scanning frequency of the digitizer. As 
    // such, we can use it to see if the user canceled their gesture mid-frame
    u16 min_scantime_diff;

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

// This updates win->trackpad_zoom and produces WIN_EVENT_TRACKPAD_ZOOM events
b32 _w32_trackpad_detect_zoom(
    window* win,
    _w32_trackpad_context* context,
    win_event* event,
    LPARAM lParam
);

void _w32_trackpad_update(_w32_trackpad_context* context);

