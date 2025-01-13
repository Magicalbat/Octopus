#include "truetype_segment.h"

vec2f tt_segment_point(const tt_segment* seg, f32 t) {
    switch (seg->type) {
        case TT_SEGMENT_LINE: {
            return line2f_point(&seg->line, t);
        } break;
        case TT_SEGMENT_QBEZIER: {
            return qbezier2f_point(&seg->qbez, t);
        } break;
    }
}
vec2f tt_segment_deriv(const tt_segment* seg, f32 t) {
    switch (seg->type) {
        case TT_SEGMENT_LINE: {
            return line2f_deriv(&seg->line);
        } break;
        case TT_SEGMENT_QBEZIER: {
            return qbezier2f_deriv(&seg->qbez, t);
        } break;
    }
}
curve_dist_info tt_segment_dist(const tt_segment* seg, vec2f target) {
    switch (seg->type) {
        case TT_SEGMENT_LINE: {
            return line2f_dist(&seg->line, target);
        } break;
        case TT_SEGMENT_QBEZIER: {
            return qbezier2f_dist(&seg->qbez, target);
        } break;
    }
}
