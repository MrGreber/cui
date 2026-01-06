#pragma once

#ifndef EDIT_H
#define EDIT_H
#include <utils.h>
#include <sprite.h>
#include <str.h>

// typedef struct glyph {
//
// } glyph_t;

typedef enum font_type {
    VCR_OSD_MONO,
} font_type_t;

typedef struct glyph {
    u32 id;
    f32 x0, y0;
    f32 x1, y1;
    struct {
        u16 width;
        u16 height;
    } dim;
    struct {
        u16 x;
        u16 y;
    } offset;
    i32 x_advance;
} glyph_t;

typedef struct font {
    u16 size;
    u16 line_height;
    struct {
        u8 vert;
        u8 horiz;
    } spacing;
    struct {
        u8 up;
        u8 right;
        u8 down;
        u8 left;
    } padding;
    color_t color;
    font_type_t type;
    glyph_t table[256];

    vert_array* va;
    vert_buf* vb;
    texture_t* tex;
} font_t;

font_t* new_font(const char* path);
void del_font(font_t* font);

typedef struct edit {
    comp_header_t header;
    style_group_t styles;

    void* parent;

    sprite_t* sprite;
    texture_t* tex;

    str_t* text;
    u64 index;
} edit_t;

edit_t* new_edit(void* parent, style_group_t* group, const bounding_box* box);
void del_edit(edit_t* edit);
void bind_edit(const edit_t* edit);
void update_edit(edit_t* edit, const mat4* projection, const f32 angle);

#endif //EDIT_H
