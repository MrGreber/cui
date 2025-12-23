#include <sprite.h>
#include <mem.h>
#include <utils.h>
#include <log.h>

#include <stdio.h>
#include <glad.h>

const static u32 indices[] = {
    2,1,0,
    2,3,1
};
const static f32 vertices[] = {
    0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f
};

sprite_t* new_sprite(const char* name) {
    buf_t buffer = {
        .size = sizeof(sprite_t),
        .tag = MEMTAG_SPRITE,
    };
    if (!new_buf(&buffer, false)) return NULL;

    sprite_t* obj = buffer.ptr;

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

    obj->shader = new_shader(name);
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
