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

#define Mesh(func) __mesh_##func
mesh_t* Mesh(new)(frame_t* frame, const mesh_tag_t tag);
bool Mesh(new_cache)(frame_t* frame);
void Mesh(del_cache)(frame_t* frame);
void Mesh(bind)(const frame_t* frame, const mesh_t* mesh);
__forceinline void Mesh(unbind)(void) {
    ElementBuffer(unbind)();
    VertexArray(unbind)();
}
void Mesh(draw)(mesh_t* mesh);

#endif // OPS_H