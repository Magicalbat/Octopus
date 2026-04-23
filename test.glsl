#version 430 core

struct dist_info {
    float sdist;
    float orthogonality;
    float param;
};

struct instance_data {
    vec2 translate;
    vec2 scale;
    uint data_offset;
    uint num_segments;
    uint num_points;
};

layout (location = 0) out vec4 out_col;

layout (binding = 0, std430) readonly buffer glyph_ssbo {
    uint glyph_packed[];
};

layout (binding = 1, std430) readonly buffer instance_ssbo {
    instance_data instances[];
};

flat in int glyph_id;
in vec2 pos;

#define POINT_FLAG_LINE        (1 << 0)
#define POINT_FLAG_CONTOUR_END (1 << 1)

#define POINT_FLAG_RED         (1 << 4)
#define POINT_FLAG_GREEN       (1 << 5)
#define POINT_FLAG_BLUE        (2 << 6)

const float INFINITY = uintBitsToFloat(0x7F800000);

float cross(vec2 a, vec2 b) {
    return a.x * b.y - a.y * b.x;
}

#define EPSILON 1e-8
#define PI 3.14159265358979323846

float cbrt(float x) {
    return sign(x) * pow(abs(x), 1.0/3.0);
}

vec2 solve_quadratic(float a, float b, float c) {
    if (abs(a) < EPSILON || abs(b) > 1e6 * abs(a)) {
        if (abs(b) < EPSILON) {
            return vec2(0.0, 1.0);
        }

        return vec2(-c / b, 0.0);
    }

    float discriminant = b * b - 4.0 * a * c;
    if (discriminant > 0.0) {
        float sqrt_discr = sqrt(discriminant);

        return vec2(
            (-b + sqrt_discr) / (2.0 * a),
            (-b - sqrt_discr) / (2.0 * a)
        );
    } else if (abs(discriminant) < EPSILON) {
        return vec2(-b / (2.0 * a), 0.0);
    }

    return vec2(0.0, 1.0);
}

// 0 = x^3 + (a2)(x^2) + (a1)(x) + a0
// https://mathworld.wolfram.com/CubicFormula.html
vec3 solve_cubic_normed(float a2, float a1, float a0) {
    float a2_2 = a2 * a2;

    float Q = (1.0/9.0) * (3.0 * a1 - a2_2);
    float R = (1.0/54.0) * (9.0 * a2 * a1 - 27.0 * a0 - 2.0 * a2_2 * a2);

    float R2 = R * R;
    float Q3 = Q * Q * Q;

    a2 *= 1.0/3.0;
    if (R2 + Q3 < 0.0) {
        float t = R / sqrt(-Q3);
        t = clamp(t, -1.0, 1.0);

        float theta = acos(t);

        float coef = 2.0 * sqrt(-Q);

        return vec3(
            coef * cos(theta / 3.0) - a2,
            coef * cos((theta + 2.0 * PI) / 3.0) - a2,
            coef * cos((theta + 4.0 * PI) / 3.0) - a2
        );
    }

    float S = cbrt(R + sqrt(R2 + Q3));
    float T = cbrt(R - sqrt(R2 + Q3));

    vec3 solutions = vec3((S + T) - a2, 1.0, 0.0);

    if (abs(S - T) < EPSILON) {
        solutions.x = -0.5f * (S + T) - a2;

        return solutions;
    }

    return solutions;
}

vec3 solve_cubic(float a, float b, float c, float d) {
    if (abs(a) >= EPSILON) {
        float b_a = b / a;
        if (abs(b_a) < 1e6f) {
            return solve_cubic_normed(b_a, c/a, d/a);
        }
    }

    return vec3(solve_quadratic(b, c, d), 0.0);
}

dist_info line_dist(vec2 p0, vec2 p1) {
    vec2 line_vec = p1 - p0;
    vec2 point_vec = pos - p0;

    float t = dot(point_vec, line_vec) / dot(line_vec, line_vec);
    t = clamp(t, 0.0, 1.0);

    vec2 line_point = p0 + line_vec * t;
    vec2 line_to_point = pos - line_point;

    float dist = length(line_to_point);

    vec2 line_dir = normalize(line_vec);
    vec2 point_dir = line_to_point / dist;

    float ortho = cross(line_dir, point_dir);
    float s = ortho < 0.0 ? -1 : 1;

    return dist_info(s * dist, s * ortho, t);
}

dist_info bez_dist(vec2 p0, vec2 p1, vec2 p2) {
    vec2 c0 = pos - p0;
    vec2 c1 = p1 - p0;
    vec2 c2 = p2 - 2.0 * p1 + p0;

    float a = dot(c2, c2);
    float b = 3.0 * dot(c1, c2);
    float c = 2.0 * dot(c1, c1) - dot(c2, c0);
    float d = -dot(c1, c0);

    vec3 ts = solve_cubic(a, b, c, d);

    float t = 0.0;
    vec2 bez_to_point = vec2(0);
    float dist = INFINITY;

    for (uint i = 0; i < 3; i++) {
        float cur_t = clamp(ts[i], 0.0, 1.0);
        vec2 cur_bez_point = cur_t * (cur_t * c2 + 2.0 * c1) + p0;
        vec2 cur_bez_to_point = pos - cur_bez_point;
        float cur_dist = length(cur_bez_to_point);

        if (cur_dist < dist) {
            t = cur_t;
            bez_to_point = cur_bez_to_point;
            dist = cur_dist;
        }
    }

    vec2 bez_dir = normalize(t * c2 + c1);
    vec2 point_dir = bez_to_point / dist;

    float ortho = cross(bez_dir, point_dir);
    float s = ortho < 0.0 ? -1 : 1;

    return dist_info(s * dist, s * ortho, t);
}

bool dist_less(dist_info a, dist_info b) {
    float a_dist = abs(a.sdist);
    float b_dist = abs(b.sdist);

    if (a_dist < b_dist) {
        return true;
    } else if (abs(a_dist - b_dist) < 1e-8) {
        return a.orthogonality > b.orthogonality;
    }

    return false;
}

uint get_flag(uint flag_index) {
    uint offset = instances[glyph_id].data_offset / 4;

    uint flag = glyph_packed[offset + flag_index / 4];
    flag = (flag >> ((flag_index % 4) * 8)) & 0xff;

    return flag;
}

vec2 get_point(uint data_index) {
    uint offset = instances[glyph_id].data_offset / 4;
    vec2 p = unpackSnorm2x16(glyph_packed[offset + data_index]) * 32727.0;

    p *= instances[glyph_id].scale;
    p += instances[glyph_id].translate;

    return p;
}

void main() {
    uint num_segments = instances[glyph_id].num_segments;
    uint num_points = instances[glyph_id].num_points;

    uint point_offset = (num_points + 3) / 4;
    uint point_index = 0;

    dist_info dist = dist_info(INFINITY, 0, 0);

    for (uint i = 0; i < num_segments; i++) {
        uint flag = get_flag(point_index);

        dist_info cur_dist = dist_info(INFINITY, 0, 0);

        if ((flag & POINT_FLAG_LINE) == POINT_FLAG_LINE) {
            vec2 p0 = get_point(point_offset + point_index++);
            vec2 p1 = get_point(point_offset + point_index);

            cur_dist = line_dist(p0, p1);
        } else {
            vec2 p0 = get_point(point_offset + point_index++);
            vec2 p1 = get_point(point_offset + point_index++);
            vec2 p2 = get_point(point_offset + point_index);

            cur_dist = bez_dist(p0, p1, p2);
        }

        flag = get_flag(point_index);
        if ((flag & POINT_FLAG_CONTOUR_END) == POINT_FLAG_CONTOUR_END) {
            point_index++;
        }

        if (dist_less(cur_dist, dist)) {
            dist = cur_dist;
        }
    }

    float d = dist.sdist;
    d = clamp(d, -10.0, 10.0);
    float outline = 1 / ((20 * d) * (20 * d) + 1);
    d = d / 20.0 + 0.5;

    out_col = vec4(outline, d, d, 1);
}

