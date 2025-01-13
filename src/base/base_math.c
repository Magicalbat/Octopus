#include "base_math.h"
#include "base_error.h"

#include <math.h>

u32 solve_quadratic(f32 solutions[2], f32 a, f32 b, f32 c) {
    if (solutions == NULL) {
        return 0;
    }

    if (a == 0 || ABS(b) > 1e6f*ABS(a)) {
        if (b == 0) {
            return 0;
        }

        solutions[0] = -c / b;
        return 1;
    }

    f32 discriminant = b * b - 4 * a * c;
    if (discriminant > 0) {
        f32 sqrt_discr = sqrtf(discriminant);

        solutions[0] = (-b + sqrt_discr) / (2.0f * a);
        solutions[1] = (-b - sqrt_discr) / (2.0f * a);

        return 2;
    } else if (discriminant == 0) {
        solutions[0] = -b / (2.0f * a);

        return 1;
    }

    return 0;
}

// 0 = x^3 + (a2)(x^2) + (a1)(x) + a0
// https://mathworld.wolfram.com/CubicFormula.html
u32 solve_cubic_normed(f32 solutions[3], f32 a2, f32 a1, f32 a0) {
    f32 a2_2 = a2 * a2;

    f32 Q = (1.0f/9.0f) * (3.0f * a1 - a2_2);
    f32 R = (1.0f/54.0f) * (9.0f * a2 * a2 - 27 * a0 - 2 * a2_2 * a2);

    f32 R2 = R * R;
    f32 Q3 = Q * Q * Q;

    a2 *= 1.0f/3.0f;
    if (R2 + Q3 < 0.0f) {
        f32 t = R / sqrtf(-Q3);
        t = CLAMP(t, -1.0f, 1.0f);

        f32 theta = acosf(t);

        f32 coef = 2.0f * sqrtf(-Q);

        solutions[0] = coef * cosf(theta / 3.0f) - a2;
        solutions[1] = coef * cosf((theta + 2.0f * PI) / 3.0f) - a2;
        solutions[2] = coef * cosf((theta + 4.0f * PI) / 3.0f) - a2;

        return 3;
    }

    f32 u = SIGN(R) * powf(ABS(R) + sqrtf(R2 + Q3), 1.0f / 3.0f);
    f32 v = u == 0.0f ? 0.0f : -Q / u;

    solutions[0] = (u + v) - a2;

    if (u == v || ABS(u - v) < 1e-6f * ABS(u + v)) {
        solutions[1] = -0.5f * (u + v) - a2;

        return 2;
    }

    return 1;
}

u32 solve_cubic(f32 solutions[3], f32 a, f32 b, f32 c, f32 d) {
    if (solutions == NULL) {
        return 0;
    }

    if (a != 0) {
        f32 b_a = b / a;
        if (ABS(b_a) < 1e6f) {
            return solve_cubic_normed(solutions, b_a, c/a, d/a);
        }
    }

    return solve_quadratic(solutions, b, c, d);
}

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
    if (len)
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
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}
vec3f vec3f_norm(vec3f v) {
    f32 len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len) {
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
    if (len) {
        f32 r = 1.0f / len;
        return (vec4f){ v.x * r, v.y * r, v.z * r, v.w * r };
    }
    return (vec4f){ 1, 0, 0, 0 };
}

mat2f mat2f_mul_mat2f(mat2f a, mat2f b) {
    mat2f out = { .m = {
        a.m[0] * b.m[0] + a.m[2] * b.m[1], 
        a.m[1] * b.m[0] + a.m[3] * b.m[1],
        a.m[0] * b.m[2] + a.m[2] * b.m[3],
        a.m[1] * b.m[2] + a.m[3] * b.m[3],
    }};

    return out;
}
vec2f mat2f_mul_vec2f(const mat2f* mat, vec2f v) {
    const f32* m = mat->m;

    return (vec2f){
        v.x * m[0] + v.y * m[1],
        v.x * m[2] + v.y * m[3]
    };
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
vec2f line2f_point(const line2f* line, f32 t) {
    vec2f out = { 0 };

    out = vec2f_add(out, vec2f_scale(line->p0, (1.0f - t)));
    out = vec2f_add(out, vec2f_scale(line->p1, t));

    return out;
}
vec2f line2f_deriv(const line2f* line) {
    return vec2f_sub(line->p1, line->p0);
}
curve_dist_info line2f_dist(const line2f* line, vec2f target) {
    vec2f line_vec = vec2f_sub(line->p1, line->p0);
    vec2f point_vec = vec2f_sub(target, line->p0);

    f32 t = vec2f_dot(point_vec, line_vec) / vec2f_dot(line_vec, line_vec);

    if (t > 0.0f && t < 1.0f) {
        vec2f line_normal = vec2f_norm(vec2f_perp(line_vec));
        f32 sdist = vec2f_dot(point_vec, line_normal);

        curve_dist_info out = { .t = t };

        if (sdist < 0) {
            out.dist = -sdist;
            out.dist_sign = -1;
        } else {
            out.dist = sdist;
            out.dist_sign = 1;
        }

        return out;
    }

    vec2f line_to_point = t < 0.5f ? vec2f_sub(line->p0, target) : vec2f_sub(line->p1, target);
    f32 dist = vec2f_len(line_to_point);
    f32 point_line_cross = vec2f_cross(line_vec, point_vec);
    f32 alignment_dot = vec2f_dot(vec2f_norm(line_vec), vec2f_norm(line_to_point));

    curve_dist_info out = {
        dist, ABS(alignment_dot),
        t > 0.5f, SIGN(point_line_cross)
    };

    return out;
}

vec2f qbezier2f_point(const qbezier2f* qbez, f32 t) {
    vec2f out = { 0 };

    out = vec2f_add(out, vec2f_scale(qbez->p0, (1.0f - t) * (1.0f - t)));
    out = vec2f_add(out, vec2f_scale(qbez->p1, 2.0f * (1.0f - t) * t));
    out = vec2f_add(out, vec2f_scale(qbez->p2, t * t));

    return out;
}
vec2f qbezier2f_deriv(const qbezier2f* qbez, f32 t) {
    vec2f out = { 0 };

    out = vec2f_add(out, vec2f_scale(qbez->p0, 2.0f * t - 2.0f));
    out = vec2f_add(out, vec2f_scale(qbez->p1, 2.0f - 4.0f * t));
    out = vec2f_add(out, vec2f_scale(qbez->p2, 2.0f * t));

    return out;

}
vec2f qbezier2f_second_deriv(const qbezier2f* qbez) {
    vec2f out = vec2f_scale(
        vec2f_add(qbez->p0, vec2f_sub(
            qbez->p2, vec2f_scale(qbez->p1, 2.0f)
        )), 2.0f
    );

    return out;
}
curve_dist_info qbezier2f_dist(const qbezier2f* qbez, vec2f target) {
    vec2f p = vec2f_sub(target, qbez->p0);
    vec2f p1 = vec2f_sub(qbez->p1, qbez->p0);
    vec2f p2 = vec2f_add(vec2f_sub(qbez->p2, vec2f_scale(qbez->p1, 2.0f)), qbez->p0);

    f32 a = vec2f_dot(p2, p2);
    f32 b = 3.0f * vec2f_dot(p1, p2);
    f32 c = 2.0f * vec2f_dot(p1, p1) - vec2f_dot(p2, p);
    f32 d = -vec2f_dot(p1, p);

    f32 ts[3] = { 0 };
    u32 num_solutions = solve_cubic(ts, a, b, c, d);

    if (num_solutions == 0) {
        ts[0] = 0.0f;
        ts[1] = 1.0f;
        num_solutions = 2;
    }

    f32 min_dist = INFINITY;
    f32 min_dist_t = 0.0f;
    vec2f nearest_point = { 0 };

    for (u32 i = 0; i < num_solutions; i++) {
        // According to a lemma in the sdf part of FreeType,
        // clamping the t's here will always handle the endpoints
        f32 t = CLAMP(ts[i], 0.0f, 1.0);
        vec2f point = qbezier2f_point(qbez, t);
        f32 dist = vec2f_dist(point, target);

        if (dist < min_dist) {
            min_dist = dist;
            min_dist_t = t;
            nearest_point = point;
        }
    }

    f32 sign_cross = vec2f_cross(qbezier2f_deriv(qbez, min_dist_t), vec2f_sub(nearest_point, target));

    curve_dist_info out = {
        .dist = min_dist,
        .dist_sign = SIGN(sign_cross),
        .t = min_dist_t
    };

    if (min_dist_t > 0.0f && min_dist_t < 1.0f) {
        return out;
    }

    f32 alignment_dot = vec2f_dot(
        vec2f_norm(qbezier2f_deriv(qbez, min_dist_t)),
        vec2f_norm(vec2f_sub(min_dist_t < 0.5f ? qbez->p0 : qbez->p2, target))
    );

    out.alignment = ABS(alignment_dot);

    return out;
}

