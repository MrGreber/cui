#pragma once

#ifndef FONT_H
#define FONT_H
#include <utils.h>
#include <texture.h>
#include <shader/types.h>
#include <str.h>
typedef struct frame frame_t;

typedef u16 (*atlas_map)(const char_t c);
typedef char_t (*key_map)(const char_t c, const bool is_shift);

typedef enum font_type {
    VCR_OSD_MONO,
} font_type;

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
    u32 glyph_count;

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

    color_t fg; // foreground
    color_t bg; // background
    font_type type;
    key_map kmap;
    atlas_map amap;
    texture_t* atlas;
    glyph_t* glyphs;
    shader_t* shader;
} font_t;

#define Font(func) __font_##func
font_t* Font(new)(frame_t* frame, const char* path);
void Font(del)(font_t* font);
void Font(bind)(const font_t* font);
void Font(set)(font_t* font, const char* path, const color_t fg, const color_t bg);

#endif //FONT_H
