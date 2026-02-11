
#define _F32_EPSILON 1e-6f

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

vec2f vec2f_perp(vec2f v) {
    return (vec2f){ -v.y, v.x };
}

f32 vec2f_cross(vec2f a, vec2f b) {
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
    f32 len = sqrtf(v.x * v.x + v.y * v.y);
    if (ABS(len) > _F32_EPSILON)
        return (vec2f){ v.x / len, v.y / len };
    return (vec2f){ 1, 0 };
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

vec3f vec3f_cross(vec3f a, vec3f b) {
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
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

vec3f vec3f_norm(vec3f v) {
    f32 len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (ABS(len) > _F32_EPSILON) {
        f32 r = 1.0f / len;
        return (vec3f){ v.x * r, v.y * r, v.z * r };
    }
    return (vec3f){ 1, 0, 0 };
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
    f32 len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    if (ABS(len) > _F32_EPSILON) {
        f32 r = 1.0f / len;
        return (vec4f){ v.x * r, v.y * r, v.z * r, v.w * r };
    }
    return (vec4f){ 1, 0, 0, 0 };
}

