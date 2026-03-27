#pragma once

#ifndef GEOMETRY_H
#define GEOMETRY_H
#include <defines.h>
#include <math-utils.h>

#include <glad.h>

typedef struct vertex_buffer {
    u32 id;
    bool dynamic;
} vert_buf_t;

typedef struct element_buffer {
    u32 id;
    u32 count;
} elem_buf_t;

typedef struct vertex_array_element {
    u32 count;
    u32 type;
    bool normalized;
} vert_elem_t;

typedef struct vertex_array {
    u32 id;
    u32 stride;
    u32 count;
    u32 capacity;
    vert_elem_t* elem;
} vert_array_t;

#define VertexBuffer(func) __vertex_buffer_##func
vert_buf_t* VertexBuffer(new)(const void* data, const u32 size, const bool dynamic);
void VertexBuffer(del)(vert_buf_t* vb);
__forceinline void VertexBuffer(bind)(const vert_buf_t* vb) {
    glBindBuffer(GL_ARRAY_BUFFER, vb->id);
}
__forceinline void VertexBuffer(unbind)(void) {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

#define ElementBuffer(func) __element_buffer_##func
elem_buf_t* ElementBuffer(new)(const u32* data, const u32 count);
void ElementBuffer(del)(elem_buf_t* eb);
__forceinline void ElementBuffer(bind)(const elem_buf_t* eb) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eb->id);
}
__forceinline void ElementBuffer(unbind)(void) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

#define VertexArray(func) __vertex_array_##func
vert_array_t* VertexArray(new)(u64 capacity);
void VertexArray(del)(vert_array_t* va);
__forceinline void VertexArray(bind)(const vert_array_t* va) {
    glBindVertexArray(va->id);
}
__forceinline void VertexArray(unbind)(void) {
    glBindVertexArray(0);
}

void VertexArray(push_f32)(vert_array_t* va, const u32 count);
void VertexArray(push_u32)(vert_array_t* va, const u32 count);
void VertexArray(push_u8)(vert_array_t* va, const u32 count);
void VertexArray(push_buffer)(vert_array_t* va, vert_buf_t* vb);

typedef struct mesh {
    u64 count;
    u64 capacity;
    vec4* vertices;

    vert_array_t* va;
    vert_buf_t* vb;
    bool dynamic;
} mesh_t;

#define Mesh(func) __mesh_##func

#endif // GEOMETRY_H