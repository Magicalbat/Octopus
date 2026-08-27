
typedef u8 win_mouse_button;
typedef u8 win_key;

typedef struct {
    v2_f32 pos; 
} win_event_mouse_move;

typedef struct {
    v2_f32 pos;
    win_mouse_button button; 
} win_event_mouse_down;

typedef struct {
    v2_f32 pos;
    win_mouse_button button; 
} win_event_mouse_up;

typedef struct {
    v2_f32 delta;
} win_event_scroll;

typedef struct {
    f32 start_dist;
    f32 dist_change;
} win_event_trackpad_zoom;

typedef struct {
    win_key key;
} win_event_key_down;

typedef struct {
    win_key key;
} win_event_key_up;

typedef struct {
    u32 id;
    u64 time_us;

    v2_f32 pos;

    // [0, 1]
    f32 pressure;
} win_touch_info;

typedef struct {
    win_touch_info touch_info;
} win_event_touch_down;

typedef struct {
    win_touch_info touch_info;
} win_event_touch_update;

typedef struct {
    win_touch_info touch_info;
} win_event_touch_up;

typedef u32 win_pen_flags;

typedef enum {
    WIN_PEN_FLAG_NONE   = 0b00,
    WIN_PEN_FLAG_ERASER = 0b01,
    WIN_PEN_FLAG_BARREL = 0b10,
} _win_pen_flags_enum;

typedef struct {
    u32 id;
    win_pen_flags flags;

    u64 time_us;

    v2_f32 pos;

    // [0, 1]
    f32 pressure;

    // From 0 to 2pi
    f32 rotation;

    // From -pi/2 to +pi/2
    v2_f32 tilt;
} win_pen_info;

typedef struct {
    win_pen_info pen_info;
} win_event_pen_down;

typedef struct {
    win_pen_info pen_info;
} win_event_pen_update;

typedef struct {
    win_pen_info pen_info;
} win_event_pen_up;

typedef struct win_event {
    struct win_event* next;

    enum {
        WIN_EVENT_NONE = 0,

        WIN_EVENT_MOUSE_MOVE,
        WIN_EVENT_MOUSE_DOWN,
        WIN_EVENT_MOUSE_UP,

        WIN_EVENT_SCROLL,

        WIN_EVENT_TRACKPAD_ZOOM,

        WIN_EVENT_KEY_DOWN,
        WIN_EVENT_KEY_UP,

        WIN_EVENT_TOUCH_DOWN,
        WIN_EVENT_TOUCH_UPDATE,
        WIN_EVENT_TOUCH_UP,

        WIN_EVENT_PEN_DOWN,
        WIN_EVENT_PEN_UPDATE,
        WIN_EVENT_PEN_UP,
    } kind;

    union {
        win_event_mouse_move mouse_move;
        win_event_mouse_down mouse_down;
        win_event_mouse_up mouse_up;

        win_event_scroll scroll;

        win_event_trackpad_zoom trackpad_zoom;

        win_event_key_down key_down;
        win_event_key_up key_up;

        win_event_touch_down touch_down;
        win_event_touch_update touch_update;
        win_event_touch_up touch_up;

        win_event_pen_down pen_down;
        win_event_pen_update pen_update;
        win_event_pen_up pen_up;
    };
} win_event;

typedef enum {
    WIN_MB_LEFT,
    WIN_MB_MIDDLE,
    WIN_MB_RIGHT, 

    WIN_MB_COUNT
} _win_mouse_button_enum;

STATIC_ASSERT(
    WIN_MB_COUNT < (1 << (sizeof(win_mouse_button) * 8)),
    num_mouse_buttons
);

typedef enum {
    WIN_KEY_NONE = 0,
    WIN_KEY_BACKSPACE,
    WIN_KEY_TAB,
    WIN_KEY_ENTER,
    WIN_KEY_CAPSLOCK,
    WIN_KEY_ESCAPE,
    WIN_KEY_SPACE,
    WIN_KEY_PAGEUP,
    WIN_KEY_PAGEDOWN,
    WIN_KEY_END,
    WIN_KEY_HOME,
    WIN_KEY_ARROW_LEFT,
    WIN_KEY_ARROW_UP,
    WIN_KEY_ARROW_RIGHT,
    WIN_KEY_ARROW_DOWN,
    WIN_KEY_INSERT,
    WIN_KEY_DELETE,
    WIN_KEY_NUMPAD_0,
    WIN_KEY_NUMPAD_1,
    WIN_KEY_NUMPAD_2,
    WIN_KEY_NUMPAD_3,
    WIN_KEY_NUMPAD_4,
    WIN_KEY_NUMPAD_5,
    WIN_KEY_NUMPAD_6,
    WIN_KEY_NUMPAD_7,
    WIN_KEY_NUMPAD_8,
    WIN_KEY_NUMPAD_9,
    WIN_KEY_NUMPAD_MULTIPLY,
    WIN_KEY_NUMPAD_ADD,
    WIN_KEY_NUMPAD_SUBTRACT,
    WIN_KEY_NUMPAD_DECIMAL,
    WIN_KEY_NUMPAD_DIVIDE,
    WIN_KEY_NUMPAD_ENTER,
    WIN_KEY_F1,
    WIN_KEY_F2,
    WIN_KEY_F3,
    WIN_KEY_F4,
    WIN_KEY_F5,
    WIN_KEY_F6,
    WIN_KEY_F7,
    WIN_KEY_F8,
    WIN_KEY_F9,
    WIN_KEY_F10,
    WIN_KEY_F11,
    WIN_KEY_F12,
    WIN_KEY_NUM_LOCK,
    WIN_KEY_SCROLL_LOCK,
    WIN_KEY_SHIFT,
    WIN_KEY_CONTROL,
    WIN_KEY_ALT,
    WIN_KEY_0 = '0',
    WIN_KEY_1 = '1',
    WIN_KEY_2 = '2',
    WIN_KEY_3 = '3',
    WIN_KEY_4 = '4',
    WIN_KEY_5 = '5',
    WIN_KEY_6 = '6',
    WIN_KEY_7 = '7',
    WIN_KEY_8 = '8',
    WIN_KEY_9 = '9',
    WIN_KEY_SEMICOLON,
    WIN_KEY_EQUAL,
    WIN_KEY_A = 'A',
    WIN_KEY_B = 'B',
    WIN_KEY_C = 'C',
    WIN_KEY_D = 'D',
    WIN_KEY_E = 'E',
    WIN_KEY_F = 'F',
    WIN_KEY_G = 'G',
    WIN_KEY_H = 'H',
    WIN_KEY_I = 'I',
    WIN_KEY_J = 'J',
    WIN_KEY_K = 'K',
    WIN_KEY_L = 'L',
    WIN_KEY_M = 'M',
    WIN_KEY_N = 'N',
    WIN_KEY_O = 'O',
    WIN_KEY_P = 'P',
    WIN_KEY_Q = 'Q',
    WIN_KEY_R = 'R',
    WIN_KEY_S = 'S',
    WIN_KEY_T = 'T',
    WIN_KEY_U = 'U',
    WIN_KEY_V = 'V',
    WIN_KEY_W = 'W',
    WIN_KEY_X = 'X',
    WIN_KEY_Y = 'Y',
    WIN_KEY_Z = 'Z',
    WIN_KEY_COMMA,
    WIN_KEY_PERIOD,
    WIN_KEY_MINUS,
    WIN_KEY_FORWARDSLASH,
    WIN_KEY_BACKSLASH,
    WIN_KEY_BACKTICK,
    WIN_KEY_LBRACKET,
    WIN_KEY_RBRACKET,
    WIN_KEY_APOSTROPHE,

    WIN_KEY_COUNT
} _win_key_enum;

STATIC_ASSERT(
    WIN_KEY_COUNT < (1 << (sizeof(win_key) * 8)),
    num_keys
);
