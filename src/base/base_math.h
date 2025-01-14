#ifndef BASE_MATH_H
#define BASE_MATH_H

#include "base_defs.h"

#define PI 3.14159265358979323846

typedef struct { f32 x, y;       } vec2f;
typedef struct { f32 x, y, z;    } vec3f;
typedef struct { f32 x, y, z, w; } vec4f;

// Row major
typedef struct { f32 m[4]; } mat2f;
typedef struct { f32 m[9]; } mat3f;

typedef struct {
    f32 x, y, w, h;
} rectf;

typedef struct {
    vec2f p0;
    vec2f p1;
} line2f;

typedef struct {
    vec2f p0;
    vec2f p1;
    vec2f p2;
} qbezier2f;

typedef struct {
    vec2f center;
    f32 aspect_ratio;
    f32 width;
    f32 rotation;
} viewf;

typedef struct {
    f32 dist;
    // Absolute value of dot product between direction of
    // segment and the vector from the segment to the target point
    // Only used when the minimum distance is to an endpoint
    f32 alignment;
    f32 t;

    // TODO: benchmark storing sign separately vs not
    i32 dist_sign;
} curve_dist_info;

// Solves 0=ax^2 + bx + c
// Returns the number of solutions
u32 solve_quadratic(f32 solutions[2], f32 a, f32 b, f32 c);
// Solves 0=ax^3 + bx^2 + cx + d
// Returns the number of solutions
u32 solve_cubic(f32 solutions[3], f32 a, f32 b, f32 c, f32 d);

vec2f vec2f_add(vec2f a, vec2f b);
vec2f vec2f_sub(vec2f a, vec2f b);
vec2f vec2f_comp_mul(vec2f a, vec2f b);
vec2f vec2f_comp_div(vec2f a, vec2f b);
vec2f vec2f_scale(vec2f v, f32 s);
vec2f vec2f_perp(vec2f v);
f32 vec2f_cross(vec2f a, vec2f b);
f32 vec2f_dot(vec2f a, vec2f b);
f32 vec2f_sqr_dist(vec2f a, vec2f b);
f32 vec2f_dist(vec2f a, vec2f b);
b32 vec2f_eq(vec2f a, vec2f b);
f32 vec2f_sqr_len(vec2f v);
f32 vec2f_len(vec2f v);
vec2f vec2f_norm(vec2f v);

vec3f vec3f_add(vec3f a, vec3f b);
vec3f vec3f_sub(vec3f a, vec3f b);
vec3f vec3f_comp_mul(vec3f a, vec3f b);
vec3f vec3f_comp_div(vec3f a, vec3f b);
vec3f vec3f_scale(vec3f v, f32 s);
vec3f vec3f_cross(vec3f a, vec3f b);
f32 vec3f_dot(vec3f a, vec3f b);
f32 vec3f_sqr_dist(vec3f a, vec3f b);
f32 vec3f_dist(vec3f a, vec3f b);
b32 vec3f_eq(vec3f a, vec3f b);
f32 vec3f_sqr_len(vec3f v);
f32 vec3f_len(vec3f v);
vec3f vec3f_norm(vec3f v);

vec4f vec4f_add(vec4f a, vec4f b);
vec4f vec4f_sub(vec4f a, vec4f b);
vec4f vec4f_comp_mul(vec4f a, vec4f b);
vec4f vec4f_comp_div(vec4f a, vec4f b);
vec4f vec4f_scale(vec4f v, f32 s);
f32 vec4f_dot(vec4f a, vec4f b);
f32 vec4f_sqr_dist(vec4f a, vec4f b);
f32 vec4f_dist(vec4f a, vec4f b);
b32 vec4f_eq(vec4f a, vec4f b);
f32 vec4f_sqr_len(vec4f v);
f32 vec4f_len(vec4f v);
vec4f vec4f_norm(vec4f v);

mat2f mat2f_mul_mat2f(mat2f a, mat2f b);
vec2f mat2f_mul_vec2f(const mat2f* mat, vec2f v);

void mat3f_transform(mat3f* mat, vec2f scale, vec2f offset, f32 rotation);
void mat3f_from_view(mat3f* mat, viewf v);
void mat3f_from_inv_view(mat3f* mat, viewf v);
vec3f mat3f_mul_vec3f(const mat3f* mat, vec3f v);
// Equivalent to mat3f_mul_vec3f(mat, { v.x, v.y, 1 }).xy;
vec2f mat3f_mul_vec2f(const mat3f* mat, vec2f v);

// Positions the rects into a rectangle of width max_width and variable height.
// The x and y's of the rects are ignored by the function and overwritten.
// The total height of the packed rects is returned
// The borders are not padded
f32 rectf_pack(rectf* rects, u32 num_rects, f32 max_width, f32 padding);

vec2f line2f_point(const line2f* line, f32 t);
vec2f line2f_deriv(const line2f* line);
curve_dist_info line2f_dist(const line2f* line, vec2f target);
f32 line2f_pseudo_sdist(const line2f* line, vec2f target);

vec2f qbezier2f_point(const qbezier2f* qbez, f32 t);
vec2f qbezier2f_deriv(const qbezier2f* qbez, f32 t);
vec2f qbezier2f_second_deriv(const qbezier2f* qbez);
curve_dist_info qbezier2f_dist(const qbezier2f* qbez, vec2f target);
f32 qbezier2f_pseudo_sdist(const qbezier2f* qbez, vec2f target);

b32 curve_dist_less(curve_dist_info a, curve_dist_info b);

#endif // BASE_MATH_H

