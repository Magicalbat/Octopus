
typedef struct {
    i16 x, y;
} v2_i16;

typedef struct {
    f32 x, y;       
} v2_f32;

typedef struct {
    f32 x, y, z;    
} v3_f32;

typedef struct {
    f32 x, y, z, w; 
} v4_f32;

typedef struct {
    f32 m[9];
} m3_f32;

typedef struct {
    v2_f32 center;
    f32 width;
    f32 aspect_ratio;
} view2_f32;

v2_f32 v2_f32_add(v2_f32 a, v2_f32 b);
v2_f32 v2_f32_sub(v2_f32 a, v2_f32 b);
v2_f32 v2_f32_comp_mul(v2_f32 a, v2_f32 b);
v2_f32 v2_f32_comp_div(v2_f32 a, v2_f32 b);
v2_f32 v2_f32_scale(v2_f32 v, f32 s);
v2_f32 v2_f32_perp(v2_f32 v);
f32 v2_f32_cross(v2_f32 a, v2_f32 b);
f32 v2_f32_dot(v2_f32 a, v2_f32 b);
f32 v2_f32_sqr_dist(v2_f32 a, v2_f32 b);
f32 v2_f32_dist(v2_f32 a, v2_f32 b);
b32 v2_f32_eq(v2_f32 a, v2_f32 b);
f32 v2_f32_sqr_len(v2_f32 v);
f32 v2_f32_len(v2_f32 v);
v2_f32 v2_f32_norm(v2_f32 v);

v3_f32 v3_f32_add(v3_f32 a, v3_f32 b);
v3_f32 v3_f32_sub(v3_f32 a, v3_f32 b);
v3_f32 v3_f32_comp_mul(v3_f32 a, v3_f32 b);
v3_f32 v3_f32_comp_div(v3_f32 a, v3_f32 b);
v3_f32 v3_f32_scale(v3_f32 v, f32 s);
v3_f32 v3_f32_cross(v3_f32 a, v3_f32 b);
f32 v3_f32_dot(v3_f32 a, v3_f32 b);
f32 v3_f32_sqr_dist(v3_f32 a, v3_f32 b);
f32 v3_f32_dist(v3_f32 a, v3_f32 b);
b32 v3_f32_eq(v3_f32 a, v3_f32 b);
f32 v3_f32_sqr_len(v3_f32 v);
f32 v3_f32_len(v3_f32 v);
v3_f32 v3_f32_norm(v3_f32 v);

v4_f32 v4_f32_add(v4_f32 a, v4_f32 b);
v4_f32 v4_f32_sub(v4_f32 a, v4_f32 b);
v4_f32 v4_f32_comp_mul(v4_f32 a, v4_f32 b);
v4_f32 v4_f32_comp_div(v4_f32 a, v4_f32 b);
v4_f32 v4_f32_scale(v4_f32 v, f32 s);
f32 v4_f32_dot(v4_f32 a, v4_f32 b);
f32 v4_f32_sqr_dist(v4_f32 a, v4_f32 b);
f32 v4_f32_dist(v4_f32 a, v4_f32 b);
b32 v4_f32_eq(v4_f32 a, v4_f32 b);
f32 v4_f32_sqr_len(v4_f32 v);
f32 v4_f32_len(v4_f32 v);
v4_f32 v4_f32_norm(v4_f32 v);

void m3_f32_transform(m3_f32* mat, v2_f32 scale, v2_f32 offset, f32 rotation);
void m3_f32_from_view2(m3_f32* mat, view2_f32 view);

