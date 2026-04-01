#include <sprite.h>
#include <memio.h>
#include <utils.h>
#include <log.h>
#include <shader/ops.h>
#include <geometry/ops.h>

#include <stdio.h>
#include <glad.h>

sprite_t* new_sprite(frame_t* frame, const shader_tag_t tag) {
    buf_t buffer = {
        .size = sizeof(sprite_t),
        .tag = MEMTAG_SPRITE,
    };
    if (!new_buf(&buffer, false)) return NULL;

    sprite_t* sprite = buffer.ptr;
    sprite->mesh = Mesh(new)(frame, RECT_MESH);
    if (!sprite->mesh) goto cleanup;
    sprite->shader = Shader(get)(frame, tag);
    sprite->tex = NULL;

    if (!sprite->shader) goto cleanup;
    return sprite;
cleanup:
    del_buf(&(buf_t){.size = sizeof(sprite_t), .tag = MEMTAG_SPRITE, .ptr = sprite});
    return NULL;
}

void del_sprite(sprite_t* sprite) {
    if (!sprite) return;
    if (sprite->tex) del_texture(sprite->tex);
    del_buf(&(buf_t){.size = sizeof(sprite_t), .tag = MEMTAG_SPRITE, .ptr = sprite});
}

void bind_sprite(const sprite_t* sprite) {
    Mesh(bind)(sprite->mesh);
    Shader(bind)(sprite->shader);
    if (sprite->tex) bind_texture(sprite->tex);
}

void unbind_sprite(void) {
    Mesh(unbind)();
}

bool set_sprite_texture(sprite_t* sprite, const u32 width, const u32 height, style_t* style) {
    if (!sprite || !style) return false;
    if (!gen_texture(&sprite->tex, &(bounding_box){0, 0, width, height}, style)) return false;
    return true;
}