#include <sprite.h>
#include <mem.h>
#include <utils.h>
#include <log.h>

#include <stdio.h>
#include <glad.h>

const static u32 indices[] = {
    0,1,2,
    2,3,0
};

sprite_t* new_sprite(const u32 x, const u32 y, const u32 width, const u32 height, const u32 frame_width, const u32 frame_height, const shader_type type) {
    buf_t buffer = {
        .size = sizeof(sprite_t),
        .tag = MEMTAG_SPRITE,
    };
    if (!new_buf(&buffer, false)) return NULL;

    sprite_t* obj = buffer.ptr;

    const f32 inv_frame_width = 1.0f / frame_width;
    const f32 inv_frame_height = 1.0f / frame_height;

    const f32 ndc_x0 = 2.0f * (f32)x * inv_frame_width - 1.0f;
    const f32 ndc_x1 = 2.0f * (f32)(x + width) * inv_frame_width - 1.0f;
    const f32 ndc_y_bottom = 1.0f - 2.0f * (f32)(y + height) * inv_frame_height;
    const f32 ndc_y_top    = 1.0f - 2.0f * (f32)y * inv_frame_height;

    const f32 vertices[] = {
        ndc_x0, ndc_y_bottom, 0.0f, 0.0f,
        ndc_x1, ndc_y_bottom, 1.0f, 0.0f,
        ndc_x1, ndc_y_top,    1.0f, 1.0f,
        ndc_x0, ndc_y_top,    0.0f, 1.0f
    };

    obj->va = new_vertex_array(2);
    obj->vb = new_vertex_buffer(vertices, sizeof(vertices));
    obj->eb = new_element_buffer(indices, sizeof(indices));
    if (!obj->va || !obj->vb || !obj->eb) goto cleanup;
    bind_vertex_array(obj->va);
    bind_vertex_buffer(obj->vb);
    bind_element_buffer(obj->eb);

    push_f32(obj->va, 2);
    push_f32(obj->va, 2);
    push_buf(obj->va, obj->vb);

    obj->shader = new_shader(type);
    if (!obj->shader) goto cleanup;
    return obj;
cleanup:
    if (obj->shader) del_shader(obj->shader);
    if (obj->va) del_vertex_array(obj->va);
    if (obj->vb) del_vertex_buffer(obj->vb);
    if (obj->eb) del_element_buffer(obj->eb);
    return NULL;
}

void del_sprite(sprite_t* obj) {
    if (!obj) return;
    if (obj->shader) del_shader(obj->shader);
    if (obj->va) del_vertex_array(obj->va);
    if (obj->vb) del_vertex_buffer(obj->vb);
    if (obj->eb) del_element_buffer(obj->eb);

    del_buf(&(buf_t){.size = sizeof(sprite_t), .tag = MEMTAG_SPRITE, .ptr = obj});
}

void bind_sprite(const sprite_t* obj) {
    bind_vertex_array(obj->va);
    glUseProgram(obj->shader->id);
}

void unbind_sprite(void) {
    unbind_vertex_array();
}
