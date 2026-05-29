#pragma once

#ifndef OBJECT_H
#define OBJECT_H
#include <geometry/types.h>
#include <shader/types.h>
#include <texture.h>
#include <frame.h>


typedef struct sprite {
    mesh_t* mesh;
    texture_t* tex;
    shader_t* shader;
} sprite_t;

#define Sprite(func) __sprite_##func
sprite_t* Sprite(new)(frame_t* frame, const shader_tag_t tag);

void Sprite(del)(sprite_t* sprite);
void Sprite(bind)(const frame_t* frame, const sprite_t* sprite);
void Sprite(unbind)(void);
bool Sprite(set_texture)(sprite_t* sprite, const u16 width, const u16 height, style_t* style);

#endif // OBJECT_H
