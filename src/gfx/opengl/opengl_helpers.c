u32 glh_create_shader(const char* vertex_source, const char* fragment_source) {
    u32 vertex_shader;
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_source, NULL);
    glCompileShader(vertex_shader);
    
    i32 success = GL_TRUE;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if(success == GL_FALSE) {
        char info_log[512];
        glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
        error_emitf("Failed to compile vertex shader: %s", info_log);
    }
    
    u32 fragment_shader;
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_source, NULL);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        char info_log[512];
        glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
        error_emitf("Failed to compile fragment shader: %s", info_log);
    }

    u32 shader_program;
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if(!success) {
        char info_log[512];
        glGetProgramInfoLog(shader_program, 512, NULL, info_log);
        error_emitf("Failed to link shader: %s", info_log);
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return shader_program;
}

u32 glh_create_buffer(u32 buffer_type, u64 size, void* data, u32 draw_type) {
    u32 buffer = 0;

    glGenBuffers(1, &buffer);
    glBindBuffer(buffer_type, buffer);
    glBufferData(buffer_type, size, data, draw_type);

    return buffer;
}

