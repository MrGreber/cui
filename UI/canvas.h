#pragma once

#ifndef CANVAS_H
#define CANVAS_H
#include <defines.h>
#include <utils.h>
#include <sprite.h>
#include <camera.h>

typedef struct brush {
    color_t color;
    f32 size;
} brush_t;

typedef struct canvas {
    comp_header_t header;

    void* parent;

    brush_t brush;
    struct {
        i32 x;
        i32 y;
    } prev;
    struct {
        u32 width;
        u32 height;
    } dim;
    struct {
        // saves the transformations
        mat4 model;
        mat4 inv_model;
        u8 init;
    } transform;


    camera_t* camera;
    sprite_t* sprite;
} canvas_t;

canvas_t* new_canvas(void* parent, const u32 width, const u32 height);
void del_canvas(canvas_t* canvas);
void bind_canvas(canvas_t* canvas);
void set_brush(canvas_t* canvas, const color_t color, const f32 size);
void update_canvas(canvas_t* canvas, const mat4* projection);

#endif //CANVAS_H
