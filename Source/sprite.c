#include <sprite.h>
#include <memio.h>
#include <utils.h>
#include <log.h>
#include <shader/ops.h>
#include <geometry/ops.h>

#include <stdio.h>
#include <glad.h>

sprite_t* Sprite(new)(frame_t* frame, const shader_tag_t tag) {
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

void Sprite(del)(sprite_t* sprite) {
    if (!sprite) return;
    if (sprite->tex) del_texture(sprite->tex);
    del_buf(&(buf_t){.size = sizeof(sprite_t), .tag = MEMTAG_SPRITE, .ptr = sprite});
}

void Sprite(bind)(const frame_t* frame, const sprite_t* sprite){
    Mesh(bind)(frame, sprite->mesh);
    Shader(bind)(sprite->shader);
    if (sprite->tex) bind_texture(sprite->tex);
}

void Sprite(unbind)(void) {
    Mesh(unbind)();
}

bool Sprite(set_texture)(sprite_t* sprite, const u32 width, const u32 height, style_t* style) {
    if (!sprite || !style) return false;
    if (!gen_texture(&sprite->tex, &(bounding_box){0, 0, width, height}, style)) return false;
    return true;
}