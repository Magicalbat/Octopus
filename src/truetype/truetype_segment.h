#ifndef TRUETYPE_SEGMENT_H
#define TRUETYPE_SEGMENT_H

#include "base/base_math.h"

typedef enum {
    TT_SEGMENT_LINE,
    TT_SEGMENT_QBEZIER
} tt_segment_type;

typedef struct {
    tt_segment_type type;

    union {
        line2f line;
        qbezier2f qbez;
    };
} tt_segment;

vec2f tt_segment_point(const tt_segment* seg, f32 t);
vec2f tt_segment_deriv(const tt_segment* seg, f32 t);
curve_dist_info tt_segment_dist(const tt_segment* seg, vec2f target);

#endif // TRUETYPE_SEGMENT_H

