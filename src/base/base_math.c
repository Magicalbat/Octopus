
#define _F32_EPSILON 1e-8f

u32 solve_quadratic(f32 solutions[2], f32 a, f32 b, f32 c) {
    if (solutions == NULL) {
        return 0;
    }

    if (ABS(a) < _F32_EPSILON || ABS(b) > 1e6f*ABS(a)) {
        if (ABS(b) < _F32_EPSILON) {
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
    } else if (ABS(discriminant) < _F32_EPSILON) {
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
    f32 R = (1.0f/54.0f) * (9.0f * a2 * a1 - 27 * a0 - 2 * a2_2 * a2);

    f32 R2 = R * R;
    f32 Q3 = Q * Q * Q;

    a2 *= 1.0f/3.0f;
    if (R2 + Q3 < 0.0f) {
        f32 t = R / sqrtf(-Q3);
        t = CLAMP(t, -1.0f, 1.0f);

        f32 theta = acosf(t);

        f32 coef = 2.0f * sqrtf(-Q);

        solutions[0] = coef * cosf(theta / 3.0f) - a2;
        solutions[1] = coef * cosf((theta + 2.0f * (f32)PI) / 3.0f) - a2;
        solutions[2] = coef * cosf((theta + 4.0f * (f32)PI) / 3.0f) - a2;

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

    if (ABS(a) >= _F32_EPSILON) {
        f32 b_a = b / a;
        if (ABS(b_a) < _F32_EPSILON) {
            return solve_cubic_normed(solutions, b_a, c/a, d/a);
        }
    }

    return solve_quadratic(solutions, b, c, d);
}

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

