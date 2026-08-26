
#define _W32_TRACKPAD_EXIT_TIMEOUT_US 5000
#define _W32_TRACKPAD_PAN_DEADZONE_MM 2
#define _W32_TRACKPAD_ZOOM_DEADZONE_MM 9

static b32 _w32_trackpad_valid_value_cap(HIDP_VALUE_CAPS* value_cap);
static b32 _w32_trackpad_to_mm(u16 units, u16 units_exp, f32* val);

_w32_trackpad_context* _w32_trackpad_init(mem_arena* arena, HWND hwnd) {
    mem_arena_temp maybe_temp = arena_temp_begin(arena);
    _w32_trackpad_context* context = PUSH_STRUCT(arena, _w32_trackpad_context);

    mem_arena_temp scratch = arena_scratch_get(&arena, 1);

    u32 num_devices = 0;
    if (GetRawInputDeviceList(
        NULL, &num_devices, sizeof(RAWINPUTDEVICELIST)
    ) != 0) {
        error_emit("[Win32 Trackpad] Failed to get device list size");
        goto error;
    }

    RAWINPUTDEVICELIST* devices = PUSH_ARRAY(
        scratch.arena, RAWINPUTDEVICELIST, num_devices
    );

    if (GetRawInputDeviceList(
        devices, &num_devices, sizeof(RAWINPUTDEVICELIST)
    ) != num_devices) {
        error_emit("[Win32 Trackpad] Failed to get device list");
        goto error;
    }

    for (u32 i = 0; i < num_devices; i++) {
        if (devices[i].dwType != RIM_TYPEHID) { continue; }

        u32 device_info_size = sizeof(RID_DEVICE_INFO);
        RID_DEVICE_INFO device_info = { 0 };
        if (GetRawInputDeviceInfo(
                devices[i].hDevice, RIDI_DEVICEINFO,
                &device_info, &device_info_size
        ) != sizeof(RID_DEVICE_INFO)) {
            error_emit("[Win32 Trackpad] Failed to get device info");
            goto error;
        }

        if (device_info.dwType != RIM_TYPEHID) { continue; }

        // Usage Page should be 0x0d (Digitizers)
        // Usage should be 0x05 (Touch Pads)
        if (
            device_info.hid.usUsagePage == 0x0d &&
            device_info.hid.usUsage == 0x05
        ) {
            context->device_handle = devices[i].hDevice;
            context->has_trackpad = true;
            break;
        }
    }

    arena_pop_to(scratch.arena, scratch.start_pos);

    if (!context->has_trackpad) {
        context->initialized = true;

        arena_scratch_release(scratch);
        return context;
    }

    RAWINPUTDEVICE rid = {
        .usUsagePage = 0x0d,
        .usUsage = 0x05,
        .dwFlags = RIDEV_INPUTSINK,
        .hwndTarget = hwnd
    };

    if (!RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE))) {
        error_emit("[Win32 Trackpad] Failed to register trackpad for raw input");

        goto error;
    }

    u32 ppd_size = 0;
    if (GetRawInputDeviceInfo(
        context->device_handle, RIDI_PREPARSEDDATA, NULL, &ppd_size
    ) != 0) {
        error_emit("[Win32 Trackpad] Failed to get trackpad preparsed data size");

        goto error;
    }

    context->ppd = (PHIDP_PREPARSED_DATA)arena_push(arena, ppd_size, false);
    if (GetRawInputDeviceInfo(
        context->device_handle, RIDI_PREPARSEDDATA, context->ppd, &ppd_size
    ) != ppd_size) {
        error_emit("[Win32 Trackpad] Failed to get trackpad preparsed data");

        goto error;
    }

    HIDP_CAPS caps = { 0 };
    if (
        HidP_GetCaps(context->ppd, &caps) != HIDP_STATUS_SUCCESS ||
        caps.UsagePage != 0x0d || caps.Usage != 0x05
    ) {
        error_emit("[Win32 Trackpad] Failed to get device capabilities");

        goto error;
    }

    context->max_link_collections = caps.NumberLinkCollectionNodes;
    context->link_collections = PUSH_ARRAY(arena, u16, context->max_link_collections);

    // This should always be greater than the necessary memory because RAWHID
    // techincally includes a one-byte array instead of a flexible array member
    // which causes the 
    context->rawinput_size = sizeof(RAWINPUTHEADER) +
        sizeof(RAWHID) + caps.InputReportByteLength;
    context->rawinput = (RAWINPUT*)arena_push(arena, context->rawinput_size, false);

    u16 num_value_caps = caps.NumberInputValueCaps;
    HIDP_VALUE_CAPS* value_caps = PUSH_ARRAY(
        scratch.arena, HIDP_VALUE_CAPS, num_value_caps
    );

    if (
        HidP_GetValueCaps(
            HidP_Input, value_caps, &num_value_caps, context->ppd
        ) != HIDP_STATUS_SUCCESS || num_value_caps != caps.NumberInputValueCaps
    ) {
        error_emit("[Win32 trackpad] Failed to get value capabilities");

        goto error;
    }

    b8 x_found = 0, y_found = 0;
    b8 id_found = 0, count_found = 0, time_found = 0;
    u32 x_idx = 0, y_idx = 0;

    for (u16 i = 0; i < num_value_caps; i++) {
        // Digitizers Usage Page
        if (value_caps[i].UsagePage == 0x0d) {
            // Contact ID Usage
            if (value_caps[i].NotRange.Usage == 0x51) {
                if (_w32_trackpad_valid_value_cap(&value_caps[i])) {
                    id_found = true;

                    u32 idx = context->num_link_collections++;
                    context->link_collections[idx] =
                        value_caps[i].LinkCollection;
                } else {
                    error_emit("[Win32 Trackpad] Unsupported contact id value capabilities");
                    goto error;
                }
            }

            // Scan Time Usage
            if (value_caps[i].NotRange.Usage == 0x56) {
                if (
                    _w32_trackpad_valid_value_cap(&value_caps[i]) &&
                    // Note(Ian)
                    // Scan Time should be unsigned
                    // https://www.usb.org/sites/default/files/hid1_11.pdf
                    // "If both the Logical Minimum and Logical Maximum extents
                    // are defined as positive values (0 or greater) then the
                    // report field can be assumed to be an unsigned value."
                    value_caps[i].LogicalMin >= 0 && 
                    value_caps[i].LogicalMax >= 0 &&
                    ((value_caps[i].Units >> 12) & 0xf) == 0x1
                ) {
                    time_found = true;
                } else {
                    error_emit("[Win32 Trackpad] Unsupported scan time value capabilities");
                    goto error;
                }
            }

            // Contact Count Usage
            if (value_caps[i].NotRange.Usage == 0x54) {
                if (
                    _w32_trackpad_valid_value_cap(&value_caps[i]) &&
                    // Contact Count should be unsigned
                    value_caps[i].LogicalMin >= 0 && 
                    value_caps[i].LogicalMax >= 0
                ) {
                    count_found = true;
                } else {
                    error_emit(
                        "[Win32 Trackpad]"
                        "Unsupported contact count value capabilities"
                    );

                    goto error;
                }
            }
        }

        // Generic Desktop Usage Page
        if (value_caps[i].UsagePage == 0x01) {
            // X Usage
            if (value_caps[i].NotRange.Usage == 0x30) {
                if (_w32_trackpad_valid_value_cap(&value_caps[i])) {
                    x_found = true;
                    x_idx = i;
                } else {
                    error_emit("[Win32 Trackpad] Unsupported x value capabilities");

                    goto error;
                }
            }

            // Y Usage
            if (value_caps[i].NotRange.Usage == 0x31) {
                if (_w32_trackpad_valid_value_cap(&value_caps[i])) {
                    y_found = true;
                    y_idx = i;
                } else {
                    error_emit("[Win32 Trackpad] Unsupported y value capabilities");

                    goto error;
                }
            }
        }
    }


    if (!x_found || !y_found || !id_found || !count_found || !time_found) {
        error_emit("[Win32 Trackpad] Unable to find basic values");

        goto error;
    }

    context->x_min_logical = (i16)value_caps[x_idx].LogicalMin;
    context->x_max_logical = (i16)value_caps[x_idx].LogicalMax;
    context->y_min_logical = (i16)value_caps[y_idx].LogicalMin;
    context->y_max_logical = (i16)value_caps[y_idx].LogicalMax;

    context->x_min_mm = (i16)value_caps[x_idx].PhysicalMin;
    context->x_max_mm = (i16)value_caps[x_idx].PhysicalMax;
    context->y_min_mm = (i16)value_caps[y_idx].PhysicalMin;
    context->y_max_mm = (i16)value_caps[y_idx].PhysicalMax;

    // Note(Ian)
    // Physical min and max defaults
    // Page 47
    // https://www.usb.org/sites/default/files/hid1_11.pdf
    // "If both the Physical Minimum and Physical Maximum extents are equal to
    // 0 then they will revert to their default interpretation.""
    if (context->x_min_mm == 0 && context->x_max_mm == 0) {
        context->x_min_mm = context->x_min_logical;
        context->x_max_mm = context->x_max_logical;
    }
    if (context->y_min_mm == 0 && context->y_max_mm == 0) {
        context->y_min_mm = context->y_min_logical;
        context->y_max_mm = context->y_max_logical;
    }

    u16 x_units = (u16)value_caps[x_idx].Units;
    u16 x_units_exp = (u16)value_caps[x_idx].UnitsExp;
    u16 y_units = (u16)value_caps[y_idx].Units;
    u16 y_units_exp = (u16)value_caps[y_idx].UnitsExp;

    if (
        !_w32_trackpad_to_mm(x_units, x_units_exp, &context->x_min_mm) ||
        !_w32_trackpad_to_mm(x_units, x_units_exp, &context->x_max_mm) ||
        !_w32_trackpad_to_mm(y_units, y_units_exp, &context->y_min_mm) ||
        !_w32_trackpad_to_mm(y_units, y_units_exp, &context->y_max_mm)
    ) {
        error_emit("[Win32 Trackpad] Invalid x and/or y physical units");

        goto error;
    }

    // Note(Ian)
    // This calculation (and subsequent gesture deadzone calculations) assume
    // that the logical units in x and the logical units in y are at the same
    // scale relative to millimeters. I do not see why any hardware would work
    // differently, but if you run into an issue here, let me know.
    f32 mm_to_logical = (f32)(context->x_max_logical - context->x_min_mm) /
        (f32)(context->x_max_mm - context->x_min_mm);

    f32 pan_deadzone = _W32_TRACKPAD_PAN_DEADZONE_MM * mm_to_logical;
    f32 zoom_deadzone = _W32_TRACKPAD_ZOOM_DEADZONE_MM *mm_to_logical;

    context->pan_sqr_deadzone = pan_deadzone * pan_deadzone;
    context->zoom_sqr_deadzone = zoom_deadzone * zoom_deadzone;

    arena_scratch_release(scratch);

    context->initialized = true;

    return context;

error:
    arena_temp_end(maybe_temp);
    arena_scratch_release(scratch);
    return NULL;
}

b32 _w32_trackpad_detect_zoom(
    _w32_trackpad_context* context,
    HWND hWnd, LPARAM lParam
);

void _w32_trackpad_update(_w32_trackpad_context* context);

// Note(Ian)
// Currently, all the values are designed to be 16 bit, not ranges,
// and not arrays. I am not sure if there are devices where certain
// values (namely Scan Time) do have a greater bit width. If you do
// run into this, please make an issue on GitHub
static b32 _w32_trackpad_valid_value_cap(HIDP_VALUE_CAPS* value_cap) {
    return
        !value_cap->IsRange &&
        value_cap->BitSize <= 16 &&
        value_cap->ReportCount == 1;
}

/* Note(Ian)
The USB HID unit system confused me for a long time,
so here is my attempt at an explanation:

The Unit system is described on pages 37-39 of 
"Device Class Definition for Human Interface Devices (HID)"
(https://www.usb.org/sites/default/files/hid1_11.pdf)

The Unit number itself is a 16 bit integer, where each nibble encodes 
different information. Nibble 0 defines which system of units should be used.
For example if (Unit & 0xf) == 0x1, that specifies this unit should use SI
linear units. Nibbles 1 through 6 define both which units to include, and 
their exponents. THESE NIBBLES CANNOT CHANGE WHICH SYSTEM OF UNITS ARE USED.
That is the part that confused me. If the first nibble is 1, the length unit
is centimeter regardless of the value of the length nibble (unless it is zero).
Additionally, these nibbles say nothing about the magnitude of the units, only
their exopnent. This is also confusing because the same table is used for
the Unit Exponent value. The final unit's magnitude is multiplied by 10 to
the Unit Exponent.

As such, when converting the x and y bounds to millimeters, I am only 
taking units values that are length based (SI or English) and with an 
exponent of 1 (measuring length, not area)
*/
static b32 _w32_trackpad_to_mm(u16 units, u16 units_exp, f32* val) {
    if (
        // Unit System must be SI Linear or English Linear
        ((units & 0xf) != 0x1 && (units & 0xf) != 0x3) ||
        // Length exponent must be 1 (not measuring area, volume, etc.) 
        ((units >> 4) & 0xf) != 0x1 || 
        units_exp > 0xf
    ) {
        return false;
    }

    static const f32 multipliers[] = {
        // Centimeters to mm
        1e1f, 1e2f, 1e3f, 1e4f, 1e5f, 1e6f, 1e7f,
        1e8f, 1e-7f, 1e-6f, 1e-5f, 1e-4f, 1e-3f,
        1e-2f, 1e-1f, 1e0f
    };

    *val *= multipliers[units_exp];

    // Units are in inches
    if ((units & 0xf) == 0x3) {
        *val *= 2.54f;
    }

    return true;
}

