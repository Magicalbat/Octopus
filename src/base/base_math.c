
#define _F32_EPSILON 1e-6f

v2_f32 v2_f32_add(v2_f32 a, v2_f32 b) {
    return (v2_f32){ a.x + b.x, a.y + b.y };
}

v2_f32 v2_f32_sub(v2_f32 a, v2_f32 b) {
    return (v2_f32){ a.x - b.x, a.y - b.y };
}

v2_f32 v2_f32_comp_mul(v2_f32 a, v2_f32 b) {
    return (v2_f32){ a.x * b.x, a.y * b.y };
}

v2_f32 v2_f32_comp_div(v2_f32 a, v2_f32 b) {
    return (v2_f32){ a.x / b.x, a.y / b.y };
}

v2_f32 v2_f32_scale(v2_f32 v, f32 s) {
    return (v2_f32){ v.x * s, v.y * s };
}

v2_f32 v2_f32_perp(v2_f32 v) {
    return (v2_f32){ -v.y, v.x };
}

f32 v2_f32_cross(v2_f32 a, v2_f32 b) {
    return a.x * b.y - a.y * b.x;
}

f32 v2_f32_dot(v2_f32 a, v2_f32 b) {
    return a.x * b.x + a.y * b.y;
}

f32 v2_f32_sqr_dist(v2_f32 a, v2_f32 b) {
    return (a.x - b.x) * (a.x - b.x) +
        (a.y - b.y) * (a.y - b.y);
}

f32 v2_f32_dist(v2_f32 a, v2_f32 b) {
    return sqrtf((a.x - b.x) * (a.x - b.x) +
        (a.y - b.y) * (a.y - b.y));
}

b32 v2_f32_eq(v2_f32 a, v2_f32 b) {
    return a.x == b.x && a.y == b.y;
}

f32 v2_f32_sqr_len(v2_f32 v) {
    return v.x * v.x + v.y * v.y;
}

f32 v2_f32_len(v2_f32 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

v2_f32 v2_f32_norm(v2_f32 v) {
    f32 len = sqrtf(v.x * v.x + v.y * v.y);
    if (ABS(len) > _F32_EPSILON)
        return (v2_f32){ v.x / len, v.y / len };
    return (v2_f32){ 1, 0 };
}

v3_f32 v3_f32_add(v3_f32 a, v3_f32 b) {
    return (v3_f32){ a.x + b.x, a.y + b.y, a.z + b.z };
}

v3_f32 v3_f32_sub(v3_f32 a, v3_f32 b) {
    return (v3_f32){ a.x - b.x, a.y - b.y, a.z - b.z };
}

v3_f32 v3_f32_comp_mul(v3_f32 a, v3_f32 b) {
    return (v3_f32){ a.x * b.x, a.y * b.y, a.z * b.z };
}

v3_f32 v3_f32_comp_div(v3_f32 a, v3_f32 b) {
    return (v3_f32){ a.x / b.x, a.y / b.y, a.z / b.z };
}

v3_f32 v3_f32_scale(v3_f32 v, f32 s) {
    return (v3_f32){ v.x * s, v.y * s, v.z * s };
}

v3_f32 v3_f32_cross(v3_f32 a, v3_f32 b) {
    return (v3_f32) {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

f32 v3_f32_dot(v3_f32 a, v3_f32 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

f32 v3_f32_sqr_dist(v3_f32 a, v3_f32 b) {
    return (a.x - b.x) * (a.x - b.x) + 
        (a.y - b.y) * (a.y - b.y) + 
        (a.z - b.z) * (a.z - b.z);
}

f32 v3_f32_dist(v3_f32 a, v3_f32 b) {
    return sqrtf((a.x - b.x) * (a.x - b.x) + 
        (a.y - b.y) * (a.y - b.y) + 
        (a.z - b.z) * (a.z - b.z));
}

b32 v3_f32_eq(v3_f32 a, v3_f32 b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

f32 v3_f32_sqr_len(v3_f32 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

f32 v3_f32_len(v3_f32 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

v3_f32 v3_f32_norm(v3_f32 v) {
    f32 len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (ABS(len) > _F32_EPSILON) {
        f32 r = 1.0f / len;
        return (v3_f32){ v.x * r, v.y * r, v.z * r };
    }
    return (v3_f32){ 1, 0, 0 };
}

v4_f32 v4_f32_add(v4_f32 a, v4_f32 b) {
    return (v4_f32){ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

v4_f32 v4_f32_sub(v4_f32 a, v4_f32 b) {
    return (v4_f32){ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

v4_f32 v4_f32_comp_mul(v4_f32 a, v4_f32 b) {
    return (v4_f32){ a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
}

v4_f32 v4_f32_comp_div(v4_f32 a, v4_f32 b) {
    return (v4_f32){ a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
}

v4_f32 v4_f32_scale(v4_f32 v, f32 s) {
    return (v4_f32){ v.x * s, v.y * s, v.z * s, v.w * s };
}

f32 v4_f32_dot(v4_f32 a, v4_f32 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

f32 v4_f32_sqr_dist(v4_f32 a, v4_f32 b) {
    return (a.x - b.x) * (a.x - b.x) + 
        (a.y - b.y) * (a.y - b.y) + 
        (a.z - b.z) * (a.z - b.z) +
        (a.w - b.w) * (a.w - b.w);
}

f32 v4_f32_dist(v4_f32 a, v4_f32 b) {
    return sqrtf((a.x - b.x) * (a.x - b.x) + 
        (a.y - b.y) * (a.y - b.y) + 
        (a.z - b.z) * (a.z - b.z) + 
        (a.w - b.w) * (a.w - b.w));
}

b32 v4_f32_eq(v4_f32 a, v4_f32 b) {
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

f32 v4_f32_sqr_len(v4_f32 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

f32 v4_f32_len(v4_f32 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

v4_f32 v4_f32_norm(v4_f32 v) {
    f32 len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    if (ABS(len) > _F32_EPSILON) {
        f32 r = 1.0f / len;
        return (v4_f32){ v.x * r, v.y * r, v.z * r, v.w * r };
    }
    return (v4_f32){ 1, 0, 0, 0 };
}

void m3_f32_transform(m3_f32* mat, v2_f32 scale, v2_f32 offset, f32 rotation) {
    f32 r_sin = sinf(rotation);
    f32 r_cos = cosf(rotation);

    *mat = (m3_f32){ .m = {
         r_cos * scale.x, r_sin * scale.x, offset.x,
        -r_sin * scale.y, r_cos * scale.y, offset.y,
         0.0f, 0.0f, 1.0f
    } };
}

void m3_f32_from_view2(m3_f32* mat, view2_f32 view) {
    v2_f32 size = { view.width, view.width / view.aspect_ratio };
    v2_f32 scale = { 2.0f / size.x, -2.0f / size.y };

    v2_f32 offset = {
        -(view.center.x * scale.x),
        -(view.center.y * scale.y)
    };

    m3_f32_transform(mat, scale, offset, 0);
}

