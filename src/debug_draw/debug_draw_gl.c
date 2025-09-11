
#define _DD_CIRCLE_BATCH_SIZE 1024
#define _DD_LINE_BATCH_SIZE 1024

typedef struct {
    u32 vert_array;
    u32 pos_pattern_buffer;
    u32 instance_buffer;

    u32 shader_prog;
    i32 mat_loc;
    // Used to ensure there is space in the geometry for aa
    i32 geom_scale_loc;
    i32 radius_loc;
    i32 col_loc;
} _dd_circles;

typedef struct {
    u32 vert_array;
    u32 pos_buffer;

    u32 shader_prog;
    i32 mat_loc;
    // Used to ensure there is space in the geometry for aa
    i32 geom_scale_loc;
    i32 radius_loc;
    i32 col_loc;
} _dd_lines;

static const char* _dd_circle_vert_source;
static const char* _dd_circle_frag_source;

static const char* _dd_line_vert_source;
static const char* _dd_line_frag_source;

static struct {
    const win_window* win;

    _dd_circles circles;
    _dd_lines lines;

    const viewf* view_ref;
    mat3f view_mat;
} _dd_state = { 0 };

f32 _dd_calc_geom_scale(f32 radius) {
    f32 screen_radius = radius * (f32)_dd_state.win->width / _dd_state.view_ref->width;
    f32 geom_scale = 1.0f + 1.0f / screen_radius;
    return geom_scale;
}

void debug_draw_init(const win_window* win) {
    _dd_state.win = win;

    // Circles init 
    {
        _dd_circles* circles = &_dd_state.circles;

        glGenVertexArrays(1, &circles->vert_array);
        glBindVertexArray(circles->vert_array);

        // +y is down because of view mat
        f32 pos_pattern[] = {
            -1.0f, -1.0f,
             1.0f, -1.0f,
            -1.0f,  1.0f,
             1.0f,  1.0f,
        };

        circles->pos_pattern_buffer = glh_create_buffer(
            GL_ARRAY_BUFFER, sizeof(pos_pattern), pos_pattern, GL_STATIC_DRAW
        );

        circles->instance_buffer = glh_create_buffer(
            GL_ARRAY_BUFFER, sizeof(vec2f) * _DD_CIRCLE_BATCH_SIZE, NULL, GL_DYNAMIC_DRAW
        );

        circles->shader_prog = glh_create_shader(
            _dd_circle_vert_source, _dd_circle_frag_source
        );

        glUseProgram(circles->shader_prog);
        circles->mat_loc = glGetUniformLocation(circles->shader_prog, "u_view_mat");
        circles->geom_scale_loc = glGetUniformLocation(circles->shader_prog, "u_geom_scale");
        circles->radius_loc = glGetUniformLocation(circles->shader_prog, "u_radius");
        circles->col_loc = glGetUniformLocation(circles->shader_prog, "u_col");
    }

    // Lines init
    {
        _dd_lines* lines = &_dd_state.lines;

        glGenVertexArrays(1, &lines->vert_array);
        glBindVertexArray(lines->vert_array);

        lines->pos_buffer = glh_create_buffer(
            GL_ARRAY_BUFFER, sizeof(vec2f) * _DD_LINE_BATCH_SIZE, NULL, GL_DYNAMIC_DRAW
        );

        lines->shader_prog = glh_create_shader(
            _dd_line_vert_source, _dd_line_frag_source
        );

        glUseProgram(lines->shader_prog);
        lines->mat_loc = glGetUniformLocation(lines->shader_prog, "u_view_mat");
        lines->geom_scale_loc = glGetUniformLocation(lines->shader_prog, "u_geom_scale");
        lines->radius_loc = glGetUniformLocation(lines->shader_prog, "u_radius");
        lines->col_loc = glGetUniformLocation(lines->shader_prog, "u_col");

    }
}

void debug_draw_destroy(void) {
    // Destroying circles
    {
        _dd_circles* circles = &_dd_state.circles;

        glDeleteProgram(circles->shader_prog);
        glDeleteBuffers(1, &circles->pos_pattern_buffer);
        glDeleteBuffers(1, &circles->instance_buffer);
        glDeleteVertexArrays(1, &circles->vert_array);
    }

    // Destroying lines
    {
        _dd_lines* lines = &_dd_state.lines;

        glDeleteProgram(lines->shader_prog);
        glDeleteBuffers(1, &lines->pos_buffer);
        glDeleteVertexArrays(1, &lines->vert_array);
    }
}

void debug_draw_set_view(const viewf* view) {
    _dd_state.view_ref = view;
    mat3f_from_view(&_dd_state.view_mat, *view);
}

void debug_draw_circles(vec2f* points, u32 num_points, f32 radius, vec4f color) {
    if (num_points == 0 || points == NULL || radius == 0.0f) {
        return;
    }

    _dd_circles* circles = &_dd_state.circles;

    glUseProgram(circles->shader_prog);
    glUniformMatrix3fv(circles->mat_loc, 1, GL_TRUE, _dd_state.view_mat.m);
    glUniform1f(circles->geom_scale_loc, _dd_calc_geom_scale(radius));
    glUniform1f(circles->radius_loc, radius);
    glUniform4f(circles->col_loc, color.x, color.y, color.z, color.w);

    glBindVertexArray(circles->vert_array);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribDivisor(1, 1);

    glBindBuffer(GL_ARRAY_BUFFER, circles->pos_pattern_buffer);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, circles->instance_buffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), (void*)0);

    u32 num_drawn = 0;
    while (num_drawn < num_points) {
        u32 cur_drawing = MIN(num_points - num_drawn, _DD_CIRCLE_BATCH_SIZE);

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec2f) * cur_drawing, points + num_drawn);

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, (GLsizei)cur_drawing);

        num_drawn += cur_drawing;
    }

    glVertexAttribDivisor(1, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

void debug_draw_lines(vec2f* points, u32 num_points, f32 radius, vec4f color) {
    if (num_points <= 1 || points == NULL || radius == 0.0f) {
        return;
    }

    _dd_lines* lines = &_dd_state.lines;

    glUseProgram(lines->shader_prog);
    glUniformMatrix3fv(lines->mat_loc, 1, GL_TRUE, _dd_state.view_mat.m);
    glUniform1f(lines->geom_scale_loc, _dd_calc_geom_scale(radius));
    glUniform1f(lines->radius_loc, radius);
    glUniform4f(lines->col_loc, color.x, color.y, color.z, color.w);

    glBindVertexArray(lines->vert_array);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribDivisor(0, 1);
    glVertexAttribDivisor(1, 1);

    glBindBuffer(GL_ARRAY_BUFFER, lines->pos_buffer);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), (void*)(0));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec2f), (void*)(sizeof(vec2f)));

    u32 num_drawn = 0;
    while (num_drawn < num_points) {
        u32 cur_drawing = MIN(num_points - num_drawn, _DD_LINE_BATCH_SIZE);

        // Reuse the last point in the current loop
        // to avoid gaps between batches
        if (num_drawn > 0) {
            num_drawn--;

            if (cur_drawing < _DD_LINE_BATCH_SIZE) {
                cur_drawing++;
            }
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec2f) * cur_drawing, points + num_drawn);

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, (GLsizei)(cur_drawing - 1));

        num_drawn += cur_drawing;
    }

    glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

static const char* _dd_circle_vert_source = GLSL_SOURCE(
    330,

    layout (location = 0) in vec2 a_pos_pattern;
    layout (location = 1) in vec2 a_pos;

    uniform mat3 u_view_mat;
    uniform float u_geom_scale;
    uniform float u_radius;

    out vec2 sdf_pos;

    void main() {
        sdf_pos = a_pos_pattern * u_geom_scale;
        vec2 world_pos = a_pos + a_pos_pattern * u_geom_scale * u_radius;
        vec2 screen_pos = (u_view_mat * vec3(world_pos, 1.0)).xy;
        gl_Position = vec4(screen_pos, 0.0, 1.0);
    }
);

static const char* _dd_circle_frag_source = GLSL_SOURCE(
    330,

    layout (location = 0) out vec4 out_col;

    uniform vec4 u_col;

    in vec2 sdf_pos;

    void main() {
        float dist = length(sdf_pos) - 1.0;
        float blending = fwidth(dist);
        float alpha = smoothstep(blending, -blending, dist);
        out_col = vec4(u_col.rgb, u_col.a * alpha);
    }
);

static const char* _dd_line_vert_source = GLSL_SOURCE(
    330,

    layout (location = 0) in vec2 a_p0;
    layout (location = 1) in vec2 a_p1;

    uniform mat3 u_view_mat;
    uniform float u_geom_scale;
    uniform float u_radius; 

    flat out vec2 p1_translated;
    out vec2 sdf_pos;

    void main() {
        vec2 line_vec = a_p1 - a_p0;
        vec2 line_dir = normalize(line_vec);
        vec2 norm = vec2(-line_dir.y, line_dir.x);

        vec2 world_pos = a_p0 + (-norm - line_dir) * u_radius * u_geom_scale;

        world_pos += float(gl_VertexID % 2) * (line_vec + line_dir * 2.0 * u_radius * u_geom_scale);
        world_pos += float(gl_VertexID < 2) * (norm * 2.0 * u_radius * u_geom_scale);

        p1_translated = a_p1 - a_p0;
        sdf_pos = world_pos - a_p0;

        vec2 screen_pos = (u_view_mat * vec3(world_pos, 1.0)).xy;
        gl_Position = vec4(screen_pos, 0.0, 1.0);
    }
);

static const char* _dd_line_frag_source = GLSL_SOURCE(
    330,

    layout (location = 0) out vec4 out_col;

    uniform vec4 u_col;
    uniform float u_radius; 

    flat in vec2 p1_translated;
    in vec2 sdf_pos;

    void main() {
        // sdf to a line with one point at the origin and second point at p1_translated
        float t_unclamped = dot(sdf_pos, p1_translated) / dot(p1_translated, p1_translated); 
        float t = clamp(t_unclamped, 0.0, 1.0);
        float dist = distance(sdf_pos, t * p1_translated) - u_radius;

        float blending = fwidth(dist);
        float alpha = smoothstep(blending, -blending, dist);
        out_col = vec4(u_col.rgb, u_col.a * alpha);
    }
);

