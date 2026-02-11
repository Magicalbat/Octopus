
typedef struct { f32 x, y;       } vec2f;
typedef struct { f32 x, y, z;    } vec3f;
typedef struct { f32 x, y, z, w; } vec4f;

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

