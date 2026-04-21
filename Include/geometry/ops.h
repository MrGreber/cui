#pragma once

#ifndef OPS_H
#define OPS_H
#include <geometry/types.h>
typedef struct frame frame_t;

#include <glad.h>

#define VertexBuffer(func) __vertex_buffer_##func
vert_buf_t VertexBuffer(new)(const bool dynamic, const void* vertices, const u32 size);
__forceinline void VertexBuffer(del)(vert_buf_t* vb) {
    const u32 id = vb->gl_id;
    glDeleteBuffers(1, &id);
    vb->gl_id = 0;
}
__forceinline void VertexBuffer(bind)(const vert_buf_t vb) {
    glBindBuffer(GL_ARRAY_BUFFER, vb.gl_id);
}
__forceinline void VertexBuffer(unbind)(void) {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

#define ElementBuffer(func) __element_buffer_##func
elem_buf_t ElementBuffer(new)(const bool dynamic, const u32* indices, const u32 size);
__forceinline void ElementBuffer(del)(elem_buf_t* eb) {
    const u32 id = eb->gl_id;
    glDeleteBuffers(1, &id);
    eb->gl_id = 0;
}
__forceinline void ElementBuffer(bind)(const elem_buf_t eb) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eb.gl_id);
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
void VertexArray(push_buffer)(vert_array_t* va, vert_buf_t vb);

typedef struct mesh_parameters {
    struct {
        mesh_tag_t tag : 24;
        u32 attributes : 8;
    };
    u32 capacity;
} mesh_param_t;
typedef struct mesh_buffer {
    struct {
        u32 count : 31;
        u32 is_vertices : 1;
    };
    u32 offset;
    const void* data;
} mesh_buf_t;
#define MESH_2D         (1 << 0)
#define MESH_3D         (1 << 1)
#define MESH_UV0        (1 << 2)
#define MESH_UV1        (1 << 3)
#define MESH_TEX_IDX    (1 << 4)
#define MESH_COLOR      (1 << 5)
#define MESH_NORM       (1 << 6)
#define MESH_EB         (1 << 7)
#define Mesh(func) __mesh_##func
mesh_t* Mesh(new)(frame_t* frame, const mesh_param_t params);
bool Mesh(new_cache)(frame_t* frame);
void Mesh(del_cache)(frame_t* frame);
void Mesh(bind)(const frame_t* frame, const mesh_t* mesh);
__forceinline void Mesh(unbind)(void) {
    ElementBuffer(unbind)();
    VertexArray(unbind)();
}
bool Mesh(write)(mesh_t* mesh, const mesh_buf_t mesh_buffer);
bool Mesh(push)(mesh_t* mesh, const mesh_buf_t mesh_buffer);
void Mesh(draw)(mesh_t* mesh);
void Mesh(sub_draw)(mesh_t* mesh, const u32 count, const u32 offset);

#endif // OPS_H