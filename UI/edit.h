#pragma once

#ifndef EDIT_H
#define EDIT_H
#include <sprite.h>
#include <font.h>
#include <str.h>

#define WRITABLE U64(0x1)

typedef struct edit {
    comp_header_t header;
    style_group_t styles;

    void* parent;

    sprite_t* sprite;
    font_t* font;
    struct {
        // saves the transformations
        mat4 model;
        u8 init;
    } transform;

    mesh_t* mesh;

    struct {
        str_t* buffer;
        u64 index;
    } text;
} edit_t;

#define Edit(func) __edit_##func
edit_t* Edit(new)(void* parent, const style_group_t* group, const bounding_box* box);
void Edit(del)(edit_t* edit);
void Edit(bind)(const edit_t* edit);
void Edit(set_text)(edit_t* edit, char_t* text, const u64 length);
void Edit(update)(edit_t* edit);

#endif //EDIT_H
