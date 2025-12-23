#pragma once
#include <defines.h>

#ifndef SHADER_H
#define SHADER_H

/**
 * @struct uniform
 * @brief Represents a shader uniform variable.
 *
 * Stores the name of the uniform and its location in the shader program.
 */
typedef struct uniform {
    char name[32];   /**< Uniform variable name (max 31 chars + null terminator) */
    u32 location;    /**< Location of the uniform in the shader program */
} uniform_t;

/**
 * @struct shader
 * @brief Represents an OpenGL shader program.
 *
 * Contains the OpenGL shader program ID.
 */
typedef struct shader {
    u32 id; /**< OpenGL shader program ID */
} shader_t;

/**
 * @brief Create a new shader program.
 * @return Pointer to the allocated shader_t, or NULL on failure
 */
shader_t* new_shader(const char* name);

/**
 * @brief Delete a shader program and free its resources.
 * @param shad Pointer to the shader to delete
 */
void del_shader(shader_t* shad);

/**
 * @brief Set an array of 4x4 matrix uniforms in a shader.
 * @param shad Pointer to the shader
 * @param var Name of the uniform variable
 * @param count Number of matrices in the array
 * @param transpose Whether to transpose the matrices (true/false)
 * @param elements Pointer to the array of matrix elements (column-major order)
 * @return true on success, false on failure
 */
bool set_mat4_uniform_array(const shader_t* shad, const char* var, const u32 count, const bool transpose, const f32* elements);

/** Convenience macro for setting a single 4x4 matrix uniform */
#define set_mat4_uniform(shad, var, transpose, elements) set_mat4_uniform_array(shad, var, 1, transpose, elements)

/**
 * @brief Set a float uniform in a shader.
 * @param shad Pointer to the shader
 * @param var Name of the uniform variable
 * @param v Float value to set
 * @return true on success, false on failure
 */
bool set_float_uniform(const shader_t* shad, const char* var, const f32 v);

/**
 * @brief Set an array of vec2 uniforms in a shader.
 * @param shad Pointer to the shader
 * @param var Name of the uniform variable
 * @param count Number of vec2 elements in the array
 * @param elements Pointer to the array of vec2 elements (x, y pairs)
 * @return true on success, false on failure
 */
bool set_vec2_uniform_array(const shader_t* shad, const char* var, const u32 count, const f32* elements);

/** Convenience macro for setting a single vec2 uniform */
#define set_vec2_uniform(shad, var, elements) set_vec2_uniform_array(shad, var, 1, elements)

#endif // SHADER_H
