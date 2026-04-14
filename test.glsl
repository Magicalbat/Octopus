#version 430 core

layout (location = 0) out vec4 out_col;

uniform uint u_num_segments;
uniform uint u_num_points;

layout (binding = 2, std430) readonly buffer ssbo {
    uint glyph_packed[];
};

in vec2 pos;

#define POINT_FLAG_LINE        (1 << 0)
#define POINT_FLAG_CONTOUR_END (1 << 1)

const float INFINITY = uintBitsToFloat(0x7F800000);

vec2 unpack_point(uint point_packed) {
    uint x_bits = (point_packed >>  0) & 0xffff;
    uint y_bits = (point_packed >> 16) & 0xffff;

    float x = -32768.0 * float((x_bits >> 15) & 1) + float(x_bits & 0x7FFF);
    float y = -32768.0 * float((y_bits >> 15) & 1) + float(y_bits & 0x7FFF);

    return vec2(x, y);
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

void main() {
    uint point_offset = (u_num_points + 3) / 4;
    uint point_index = 0;

    float min_dist = INFINITY;

    for (uint i = 0; i < u_num_segments; i++) {
        uint flag = (glyph_packed[point_index / 4] >> ((point_index % 4) * 8)) & 0xff;

        if ((flag & POINT_FLAG_LINE) == POINT_FLAG_LINE) {
            uint p0_packed = glyph_packed[point_offset + point_index++];
            uint p1_packed = glyph_packed[point_offset + point_index];

            vec2 p0 = unpack_point(p0_packed);
            vec2 p1 = unpack_point(p1_packed);

            p0.y *= -1;
            p1.y *= -1;

            vec2 line_vec = p1 - p0;
            vec2 point_vec = pos - p0;

            float t = dot(point_vec, line_vec) / dot(line_vec, line_vec);
            t = clamp(t, 0.0, 1.0);

            vec2 line_point = p0 + line_vec * t;
            min_dist = min(min_dist, distance(pos, line_point));
        } else {
            uint p0_packed = glyph_packed[point_offset + point_index++];
            uint p1_packed = glyph_packed[point_offset + point_index++];
            uint p2_packed = glyph_packed[point_offset + point_index];

            vec2 p0 = unpack_point(p0_packed);
            vec2 p1 = unpack_point(p1_packed);
            vec2 p2 = unpack_point(p2_packed);

            p0.y *= -1;
            p1.y *= -1;
            p2.y *= -1;

            vec2 c0 = pos - p0;
            vec2 c1 = p1 - p0;
            vec2 c2 = p2 - 2.0 * p1 + p0;

            float a = dot(c2, c2);
            float b = 3.0 * dot(c1, c2);
            float c = 2.0 * dot(c1, c1) - dot(c2, c0);
            float d = -dot(c1, c0);

            vec3 ts = solve_cubic(a, b, c, d);

            for (uint i = 0; i < 3; i++) {
                float t = clamp(ts[i], 0.0, 1.0);
                vec2 bez_point = t * t * c2 + 2.0 * t * c1 + p0;
                min_dist = min(min_dist, distance(pos, bez_point));
            }
        }

        flag = (glyph_packed[point_index / 4] >> ((point_index % 4) * 8)) & 0xff;
        if ((flag & POINT_FLAG_CONTOUR_END) == POINT_FLAG_CONTOUR_END) {
            point_index++;
        }
    }

    float dist = min_dist - 10.0;
    float blending = length(vec2(dFdx(dist), dFdy(dist))) * 0.573896787348;
    float alpha = smoothstep(blending, -blending, dist);
    
    out_col = vec4(alpha, 0., 0., 1.);
}

