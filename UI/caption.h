#pragma once

#ifndef CAPTION_H
#define CAPTION_H
#include <sprite.h>

typedef struct caption {
    comp_header_t header;

    void* parent;
    sprite_t* sprite;
    struct {
        i32 x, y;
    } prev;

    mat4 model;
} caption_t;

#define Caption(func) __caption_##func
caption_t* Caption(new)(void* parent);
void Caption(del)(caption_t* caption);
void Caption(bind)(const caption_t* caption);
void Caption(update)(caption_t* caption);

#endif // CAPTION_H
