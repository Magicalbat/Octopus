#ifndef TRUETYPE_SEGMENT_H
#define TRUETYPE_SEGMENT_H

#include "base/base_math.h"

typedef enum {
    TT_SEGMENT_LINE,
    TT_SEGMENT_QBEZIER
} tt_segment_type;

typedef enum {
    TT_SEGMENT_FLAG_CONTOUR_START = (1 << 0),

    TT_SEGMENT_FLAG_RED = (1 << 1),
    TT_SEGMENT_FLAG_GREEN = (1 << 2),
    TT_SEGMENT_FLAG_BLUE = (1 << 3),
} tt_segment_flag;

typedef struct {
    tt_segment_type type;

    union {
        line2f line;
        qbezier2f qbez;
    };

    u32 flags;
} tt_segment;

vec2f tt_segment_point(const tt_segment* seg, f32 t);
vec2f tt_segment_deriv(const tt_segment* seg, f32 t);
curve_dist_info tt_segment_dist(const tt_segment* seg, vec2f target);

#endif // TRUETYPE_SEGMENT_H

