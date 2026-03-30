#include <sprite.h>
#include <memio.h>
#include <utils.h>
#include <log.h>
#include <shader/ops.h>
#include <geometry/ops.h>

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

sprite_t* new_sprite(frame_t* frame, const shader_tag_t tag) {
    buf_t buffer = {
        .size = sizeof(sprite_t),
        .tag = MEMTAG_SPRITE,
    };
    if (!new_buf(&buffer, false)) return NULL;

    sprite_t* sprite = buffer.ptr;

    sprite->eb = ElementBuffer(new)();
    ElementBuffer(set)(sprite->eb, indices, sizeof(indices));

    sprite->vb = VertexBuffer(new)(false);
    sprite->va = VertexArray(new)(2);
    VertexArray(bind)(sprite->va);
    VertexArray(push_f32)(sprite->va, 2);
    VertexArray(push_f32)(sprite->va, 2);
    VertexArray(push_buffer)(sprite->va, sprite->vb);

    VertexBuffer(bind)(sprite->vb);
    VertexBuffer(set)(sprite->vb, vertices, sizeof(vertices));
    if (!sprite->va || !sprite->vb.id || !sprite->eb.id) goto cleanup;


    sprite->shader = Shader(get)(frame, tag);
    sprite->tex = NULL;

    if (!sprite->shader) goto cleanup;
    return sprite;
cleanup:
    if (sprite->va) VertexArray(del)(sprite->va);
    if (sprite->vb.id) VertexBuffer(del)(&sprite->vb);
    if (sprite->eb.id) ElementBuffer(del)(&sprite->eb);
    del_buf(&(buf_t){.size = sizeof(sprite_t), .tag = MEMTAG_SPRITE, .ptr = sprite});
    return NULL;
}

void del_sprite(sprite_t* sprite) {
    if (!sprite) return;
    if (sprite->tex) del_texture(sprite->tex);
    if (sprite->va) VertexArray(del)(sprite->va);
    if (sprite->vb.id) VertexBuffer(del)(&sprite->vb);
    if (sprite->eb.id) ElementBuffer(del)(&sprite->eb);

    del_buf(&(buf_t){.size = sizeof(sprite_t), .tag = MEMTAG_SPRITE, .ptr = sprite});
}

void bind_sprite(const sprite_t* sprite) {
    VertexArray(bind)(sprite->va);
    ElementBuffer(bind)(sprite->eb);
    glUseProgram(sprite->shader->id);
    if (sprite->tex) bind_texture(sprite->tex);
}

void unbind_sprite(void) {
    VertexArray(unbind)();
}

bool set_sprite_texture(sprite_t* sprite, const u32 width, const u32 height, style_t* style) {
    if (!sprite || !style) return false;
    if (!gen_texture(&sprite->tex, &(bounding_box){0, 0, width, height}, style)) return false;
    return true;
}