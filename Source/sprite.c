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

sprite_t* new_sprite(const char* shader_name) {
    buf_t buffer = {
        .size = sizeof(sprite_t),
        .tag = MEMTAG_SPRITE,
    };
    if (!new_buf(&buffer, false)) return NULL;

    sprite_t* sprite = buffer.ptr;

    sprite->va = new_vertex_array(2);
    sprite->vb = new_vertex_buffer(vertices, sizeof(vertices), STATIC_BUFFER);
    sprite->eb = new_element_buffer(indices, sizeof(indices));
    if (!sprite->va || !sprite->vb || !sprite->eb) goto cleanup;
    bind_vertex_array(sprite->va);
    bind_vertex_buffer(sprite->vb);
    bind_element_buffer(sprite->eb);

    push_f32(sprite->va, 2);
    push_f32(sprite->va, 2);
    push_buf(sprite->va, sprite->vb);

    sprite->shader = new_shader(shader_name);
    sprite->tex = NULL;

    if (!sprite->shader) goto cleanup;
    return sprite;
cleanup:
    if (sprite->shader) del_shader(sprite->shader);
    if (sprite->va) del_vertex_array(sprite->va);
    if (sprite->vb) del_vertex_buffer(sprite->vb);
    if (sprite->eb) del_element_buffer(sprite->eb);
    return NULL;
}

void del_sprite(sprite_t* sprite) {
    if (!sprite) return;
    if (sprite->tex) del_texture(sprite->tex);
    if (sprite->shader) del_shader(sprite->shader);
    if (sprite->va) del_vertex_array(sprite->va);
    if (sprite->vb) del_vertex_buffer(sprite->vb);
    if (sprite->eb) del_element_buffer(sprite->eb);

    del_buf(&(buf_t){.size = sizeof(sprite_t), .tag = MEMTAG_SPRITE, .ptr = sprite});
}

void bind_sprite(const sprite_t* sprite) {
    bind_vertex_array(sprite->va);
    glUseProgram(sprite->shader->id);
    if (sprite->tex) bind_texture(sprite->tex);

}

void unbind_sprite(void) {
    unbind_vertex_array();
}

bool set_sprite_texture(sprite_t* sprite, const u32 width, const u32 height, style_t* style) {
    if (!sprite || !style) return false;
    if (!gen_texture(&sprite->tex, &(bounding_box){0, 0, width, height}, style)) return false;
    return true;
}