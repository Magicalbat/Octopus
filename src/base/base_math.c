#include "base_math.h"
#include "base_error.h"

#include <math.h>

vec2f vec2f_add(vec2f a, vec2f b) {
    return (vec2f){ a.x + b.x, a.y + b.y };
}
vec2f vec2f_sub(vec2f a, vec2f b) {
    return (vec2f){ a.x - b.x, a.y - b.y };
}
vec2f vec2f_comp_mul(vec2f a, vec2f b) {
    return (vec2f){ a.x * b.x, a.y * b.y };
}
vec2f vec2f_comp_div(vec2f a, vec2f b) {
    return (vec2f){ a.x / b.x, a.y / b.y };
}
vec2f vec2f_scale(vec2f v, f32 s) {
    return (vec2f){ v.x * s, v.y * s };
}
f32 vec2f_crs(vec2f a, vec2f b) {
    return a.x * b.y - a.y * b.x;
}
f32 vec2f_dot(vec2f a, vec2f b) {
    return a.x * b.x + a.y * b.y;
}
f32 vec2f_sqr_dist(vec2f a, vec2f b) {
    return (a.x - b.x) * (a.x - b.x) +
        (a.y - b.y) * (a.y - b.y);
}
f32 vec2f_dist(vec2f a, vec2f b) {
    return sqrtf((a.x - b.x) * (a.x - b.x) +
        (a.y - b.y) * (a.y - b.y));
}
b32 vec2f_eq(vec2f a, vec2f b) {
    return a.x == b.x && a.y == b.y;
}
f32 vec2f_sqr_len(vec2f v) {
    return v.x * v.x + v.y * v.y;
}
f32 vec2f_len(vec2f v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}
vec2f vec2f_norm(vec2f v) {
    f32 r = 1.0f / sqrtf(v.x * v.x + v.y * v.y);
    return (vec2f){ v.x * r, v.y * r };
}

vec3f vec3f_add(vec3f a, vec3f b) {
    return (vec3f){ a.x + b.x, a.y + b.y, a.z + b.z };
}
vec3f vec3f_sub(vec3f a, vec3f b) {
    return (vec3f){ a.x - b.x, a.y - b.y, a.z - b.z };
}
vec3f vec3f_comp_mul(vec3f a, vec3f b) {
    return (vec3f){ a.x * b.x, a.y * b.y, a.z * b.z };
}
vec3f vec3f_comp_div(vec3f a, vec3f b) {
    return (vec3f){ a.x / b.x, a.y / b.y, a.z / b.z };
}
vec3f vec3f_scale(vec3f v, f32 s) {
    return (vec3f){ v.x * s, v.y * s, v.z * s };
}
vec3f vec3f_crs(vec3f a, vec3f b) {
    return (vec3f) {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}
f32 vec3f_dot(vec3f a, vec3f b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
f32 vec3f_sqr_dist(vec3f a, vec3f b) {
    return (a.x - b.x) * (a.x - b.x) + 
        (a.y - b.y) * (a.y - b.y) + 
        (a.z - b.z) * (a.z - b.z);
}
f32 vec3f_dist(vec3f a, vec3f b) {
    return sqrtf((a.x - b.x) * (a.x - b.x) + 
        (a.y - b.y) * (a.y - b.y) + 
        (a.z - b.z) * (a.z - b.z));
}
b32 vec3f_eq(vec3f a, vec3f b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
f32 vec3f_sqr_len(vec3f v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}
f32 vec3f_len(vec3f v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}
vec3f vec3f_norm(vec3f v) {
    f32 r = 1.0f / sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return (vec3f){ v.x * r, v.y * r, v.z * r };
}

vec4f vec4f_add(vec4f a, vec4f b) {
    return (vec4f){ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}
vec4f vec4f_sub(vec4f a, vec4f b) {
    return (vec4f){ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}
vec4f vec4f_comp_mul(vec4f a, vec4f b) {
    return (vec4f){ a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
}
vec4f vec4f_comp_div(vec4f a, vec4f b) {
    return (vec4f){ a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
}
vec4f vec4f_scale(vec4f v, f32 s) {
    return (vec4f){ v.x * s, v.y * s, v.z * s, v.w * s };
}
f32 vec4f_dot(vec4f a, vec4f b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
f32 vec4f_sqr_dist(vec4f a, vec4f b) {
    return (a.x - b.x) * (a.x - b.x) + 
        (a.y - b.y) * (a.y - b.y) + 
        (a.z - b.z) * (a.z - b.z) +
        (a.w - b.w) * (a.w - b.w);
}
f32 vec4f_dist(vec4f a, vec4f b) {
    return sqrtf((a.x - b.x) * (a.x - b.x) + 
        (a.y - b.y) * (a.y - b.y) + 
        (a.z - b.z) * (a.z - b.z) + 
        (a.w - b.w) * (a.w - b.w));
}
b32 vec4f_eq(vec4f a, vec4f b) {
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}
f32 vec4f_sqr_len(vec4f v) {
    return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}
f32 vec4f_len(vec4f v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}
vec4f vec4f_norm(vec4f v) {
    f32 r = 1.0f / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    return (vec4f){ v.x * r, v.y * r, v.z * r, v.w * r };
}

void mat3f_transform(mat3f* mat, vec2f scale, vec2f offset, f32 rotation) {
    f32 r_sin = sinf(rotation);
    f32 r_cos = cosf(rotation);

    *mat = (mat3f){ .m = {
         r_cos * scale.x, r_sin * scale.x, offset.x,
        -r_sin * scale.y, r_cos * scale.y, offset.y,
         0.0f, 0.0f, 1.0f
    } };
}

void mat3f_from_view(mat3f* mat, viewf v) {
    vec2f size = vec2f_scale((vec2f){ 1.0f, 1.0f / v.aspect_ratio }, v.width);

    vec2f scale = {
         2.0f / size.x,
        -2.0f / size.y,
    };

    f32 r_sin = sinf(v.rotation);
    f32 r_cos = cosf(v.rotation);

    vec2f offset = {
        -(v.center.x * (r_cos * scale.x) + v.center.y * (r_sin * scale.x)),
        -(v.center.x * (-r_sin * scale.y) + v.center.y * (r_cos * scale.y))
    };

    mat3f_transform(mat, scale, offset, v.rotation);
}

void mat3f_from_inv_view(mat3f* mat, viewf v) {
    vec2f size = vec2f_scale((vec2f){ 1.0f, 1.0f / v.aspect_ratio }, v.width);

    vec2f scale = {
         size.x / 2.0f,
        -size.y / 2.0f,
    };

    mat3f_transform(mat, scale, v.center, v.rotation);
}

vec3f mat3f_mul_vec3f(const mat3f* mat, vec3f v) {
    const f32* m = mat->m;

    vec3f out = {
        v.x * m[0] + v.y * m[1] + v.z * m[2],
        v.x * m[3] + v.y * m[4] + v.z * m[5],
        v.x * m[6] + v.y * m[7] + v.z * m[8]
    };

    return out;
}
vec2f mat3f_mul_vec2f(const mat3f* mat, vec2f v) {
    vec3f v3 = { v.x, v.y, 1 };
    v3 = mat3f_mul_vec3f(mat, v3);
    return (vec2f){ v3.x, v3.y };
}

