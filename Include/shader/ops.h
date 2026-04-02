#pragma once

#ifndef SHADER_OPS_H
#define SHADER_OPS_H
#include <frame.h>
#include <shader/types.h>

#include <glad.h>

#define Shader(func) __shader_##func
shader_t* Shader(get)(frame_t* frame, const shader_tag_t tag);
bool Shader(new_cache)(frame_t* frame);
void Shader(del_cache)(frame_t* frame);
__forceinline void Shader(bind)(const shader_t* shader) {
    glUseProgram(shader->id);
}

bool Shader(set_mat4_array)(shader_t* shader, const char* name, const u32 count, const bool transpose, const f32* elements);
__forceinline bool Shader(set_mat4)(shader_t* shader, const char* name, const bool transpose, const f32* elements) {
    return Shader(set_mat4_array)(shader, name, 1, transpose, elements);
}

bool Shader(set_float)(shader_t* shader, const char* name, const f32 value);

bool Shader(set_vec2_array)(shader_t* shader, const char* name, const u32 count, const f32* elements);
__forceinline bool Shader(set_vec2)(shader_t* shader, const char* name, const f32* elements) {
    return Shader(set_vec2_array)(shader, name, 1, elements);
}

bool Shader(set_vec4_array)(shader_t* shader, const char* name, const u32 count, const f32* elements);
__forceinline bool Shader(set_vec4)(shader_t* shader, const char* name, const f32* elements) {
    return Shader(set_vec4_array)(shader, name, 1, elements);
}


#endif // SHADER_OPS_H