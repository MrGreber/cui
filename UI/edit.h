#pragma once

#ifndef EDIT_H
#define EDIT_H
#include <utils.h>
#include <sprite.h>
#include <str.h>

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

    color_t fg;
    color_t bg;
    font_type_t type;

    // ToDo: create a mesh module
    struct {
        u64 count;
        u64 capacity;
        vec4* vertices;

        vert_array* va;
        vert_buf* vb;
    } mesh;
    texture_t* atlas;
    shader_t* shader;

    // ToDo: make this heap allocated
    glyph_t table[256];
} font_t;

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

    struct {
        str_t* buffer;
        u64 index;
    } text;
} edit_t;

edit_t* new_edit(void* parent, const style_group_t* group, const bounding_box* box);
void del_edit(edit_t* edit);
void bind_edit(const edit_t* edit);
void set_font(edit_t* edit, const char* path, const color_t fg, const color_t bg);
void update_edit(edit_t* edit, const mat4* projection, const f32 angle);
void unfocus_edit(edit_t* edit);

#endif //EDIT_H
