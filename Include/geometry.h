#pragma once
#include <defines.h>

#ifndef GEOMETRY_H
#define GEOMETRY_H

/**
 * OpenGL vertex buffer object (VBO) wrapper.
 */
typedef struct vertex_buffer {
    u32 id; /**< OpenGL-generated buffer ID */
} vert_buf;

/**
 * OpenGL element/index buffer object (EBO) wrapper.
 */
typedef struct element_buffer {
    u32 id;    /**< OpenGL-generated buffer ID */
    u32 count; /**< Number of elements in the buffer */
} elem_buf;

/**
 * Represents a single element (attribute) in a vertex array.
 */
typedef struct vertex_array_element {
    u32 count;      /**< Number of components (e.g., 3 for vec3) */
    u32 type;       /**< OpenGL type (e.g., GL_FLOAT) */
    bool normalized;/**< Whether the values are normalized */
} vert_elem;

/**
 * OpenGL vertex array object (VAO) wrapper.
 */
typedef struct vertex_array {
    u32 id;          /**< OpenGL-generated VAO ID */
    u32 stride;      /**< Total stride of one vertex in bytes */
    u32 count;       /**< Number of elements added */
    u32 capacity;    /**< Maximum number of elements before resizing needed */
    vert_elem* elem; /**< Array of vertex attribute descriptors */
} vert_array;

/**
 * Create a new vertex buffer and upload data to GPU.
 * @param data Pointer to vertex data
 * @param size Size of data in bytes
 * @return Pointer to allocated vertex buffer, or NULL on failure
 */
vert_buf* new_vertex_buffer(const void* data, const u32 size);

/**
 * Delete a vertex buffer and free GPU resources.
 * @param vb Pointer to vertex buffer
 */
void del_vertex_buffer(vert_buf* vb);

/**
 * Bind a vertex buffer for subsequent OpenGL operations.
 * @param vb Pointer to vertex buffer
 */
void bind_vertex_buffer(const vert_buf* vb);

/**
 * Unbind the currently bound vertex buffer.
 */
void unbind_vertex_buffer();

/**
 * Create a new element (index) buffer and upload data to GPU.
 * @param data Pointer to element/index data
 * @param count Number of indices
 * @return Pointer to allocated element buffer, or NULL on failure
 */
elem_buf* new_element_buffer(const u32* data, const u32 count);

/**
 * Delete an element buffer and free GPU resources.
 * @param eb Pointer to element buffer
 */
void del_element_buffer(elem_buf* eb);

/**
 * Bind an element buffer for subsequent OpenGL draw calls.
 * @param vb Pointer to element buffer
 */
void bind_element_buffer(const elem_buf* vb);

/**
 * Unbind the currently bound element buffer.
 */
void unbind_element_buffer();

/**
 * Create a new vertex array object (VAO).
 * @param capacity Initial capacity for vertex attributes
 * @return Pointer to allocated vertex array, or NULL on failure
 */
vert_array* new_vertex_array(const u64 capacity);

/**
 * Delete a vertex array and free GPU resources.
 * @param va Pointer to vertex array
 */
void del_vertex_array(vert_array* va);

/**
 * Bind a vertex array for subsequent OpenGL operations.
 * @param va Pointer to vertex array
 */
void bind_vertex_array(const vert_array* va);

/**
 * Unbind the currently bound vertex array.
 */
void unbind_vertex_array();

/**
 * Add a vertex attribute of type float to the vertex array.
 * @param va Pointer to vertex array
 * @param count Number of components (e.g., 2 for vec2)
 */
void push_f32(vert_array* va, const u32 count);

/**
 * Add a vertex attribute of type unsigned int to the vertex array.
 * @param va Pointer to vertex array
 * @param count Number of components
 */
void push_u32(vert_array* va, const u32 count);

/**
 * Add a vertex attribute of type unsigned byte to the vertex array.
 * @param va Pointer to vertex array
 * @param count Number of components
 */
void push_u8(vert_array* va, const u32 count);

/**
 * Associate a vertex buffer with the vertex array.
 * @param va Pointer to vertex array
 * @param vb Pointer to vertex buffer
 */
void push_buf(vert_array* va, vert_buf* vb);

#endif // GEOMETRY_H
