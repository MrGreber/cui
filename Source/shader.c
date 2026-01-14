#include <shader.h>
#include <mem.h>
#include <log.h>
#include <utils.h>

#include <glad.h>
#include <stdio.h>

static bool __compile_shader(const u32 type, const char* path, u32* id) {
    char* source = NULL;
    u64 size = 0;
    if (!read_file(path, &source, &size)) {
        logFatal("__compile_shader - Failed to read shader.");
        return false;
    }

    const u32 _id = glCreateShader(type);
    if (!_id) goto cleanup;
    glcall(glShaderSource(_id, 1, &source, NULL), cleanup, "__compile_shader - Failed to build shader: %s.", path);
    glcall(glCompileShader(_id), cleanup, "__compile_shader - Failed to compile shader: %s.", path);

    i32 success = 0;
    glcall(glGetShaderiv(_id, GL_COMPILE_STATUS, &success), cleanup, "__compile_shader - Failed to get shader: %s iv.", path);
    if (!success) {
        char msg[512] = { 0 };
        glGetShaderInfoLog(_id, 512, NULL, msg);
        logError("__compile_shader - shader compilation error:\n%s", msg);
        goto cleanup;
    }

    *id = _id;
    del_buf(&(buf_t){.size = size, .tag = MEMTAG_BYTE, .ptr = source});
    return true;
cleanup:
    if (source) del_buf(&(buf_t){.size = size, .tag = MEMTAG_BYTE, .ptr = source});
    return false;
}
static unsigned int __link_shader_program(const char* vertex_path, const char* fragment_path, u32* id) {
    u32 vert_id = 0;
    u32 frag_id = 0;
    if (!__compile_shader(GL_VERTEX_SHADER, vertex_path, &vert_id) || !__compile_shader(GL_FRAGMENT_SHADER, fragment_path, &frag_id)) {
        logFatal("Failed to compile vertex/fragment shader.");
        goto cleanup;
    }

    const u32 _id = glCreateProgram();
    if (!_id) goto cleanup;
    glcall(glAttachShader(_id, vert_id), cleanup, "__create_shader_program - Failed to attach vertex shader.");
    glcall(glAttachShader(_id, frag_id), cleanup, "__create_shader_program - Failed to attach fragment shader.");
    glcall(glLinkProgram(_id), cleanup, "__create_shader_program - Failed to link shader.");

    i32 success = 0;
    glcall(glGetProgramiv(_id, GL_LINK_STATUS, &success), cleanup, "__compile_shader - Failed to get shader iv.");

    *id = _id;
    glDeleteShader(vert_id);
    glDeleteShader(frag_id);
    return true;
cleanup:
    glDeleteShader(vert_id);
    glDeleteShader(frag_id);
    glDeleteProgram(_id);
    return false;
}


#define SHADER_DIR __DIR__"\\Shader\\"
shader_t* new_shader(const char* name) {
    buf_t buffer = {
        .size = sizeof(shader_t),
        .tag = MEMTAG_SHADER,
    };
    if (!new_buf(&buffer, false)) return NULL;

    shader_t* shad = buffer.ptr;

    char vertex_path[256] = { 0 };
    char fragment_path[256] = { 0 };
    sprintf_s(vertex_path, 256, SHADER_DIR"%s.vert", name);
    sprintf_s(fragment_path, 256, SHADER_DIR"%s.frag", name);


    if (!__link_shader_program(
        vertex_path,
        fragment_path,
        &shad->id
    )) {
        logFatal("new_shader - Failed to complie shader.");
        del_buf(&buffer);
        return NULL;
    }
    glUseProgram(shad->id);
    return shad;
}
void del_shader(shader_t* shad) {
    if (!shad) return;
    glDeleteProgram(shad->id);
    del_buf(&(buf_t){.size = sizeof(shader_t), .tag = MEMTAG_SHADER, .ptr = shad});
}

static i32 __get_uniform_location(const u32 id, const char* var) {
    const i32 location = glGetUniformLocation(id, var);
    if (location == -1) {
        logFatal("__get_uniform_location - Failed to find uniform: %s.", var);
        return -1;
    }
    return location;
}
bool set_mat4_uniform_array(const shader_t* shad, const char* var, const u32 count, const bool transpose, const f32* elements) {
    const i32 location = __get_uniform_location(shad->id, var);
    if (location == -1) return false;

    glcall(glUniformMatrix4fv(location, count, transpose, elements), cleanup, "set_mat4_uniform_array - Failed to set matrix uniform.");
    return true;
cleanup:
    return false;
}
bool set_float_uniform(const shader_t* shad, const char* var, const f32 v) {
    const i32 location = __get_uniform_location(shad->id, var);
    if (location == -1) return false;

    glcall(glUniform1f(location, v), cleanup, "set_float_uniform - Failed to set float uniform.");
    return true;
cleanup:
    return false;
}

bool set_vec2_uniform_array(const shader_t* shad, const char* var, const u32 count, const f32* elements) {
    const i32 location = __get_uniform_location(shad->id, var);
    if (location == -1) return false;

    glcall(glUniform2fv(location, count, elements), cleanup, "set_vec2_uniform_array - Failed to set float uniform.");
    return true;
cleanup:
    return false;
}

bool set_vec4_uniform_array(const shader_t* shad, const char* var, const u32 count, const f32* elements) {
    const i32 location = __get_uniform_location(shad->id, var);
    if (location == -1) return false;

    glcall(glUniform4fv(location, count, elements), cleanup, "set_vec4_uniform_array - Failed to set float uniform.");
    return true;
    cleanup:
        return false;
}