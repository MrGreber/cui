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

#define Canvas(func) __canvas_##func
canvas_t* Canvas(new)(void* parent, const u32 width, const u32 height);
void Canvas(del)(canvas_t* canvas);
void Canvas(bind)(canvas_t* canvas);
void Canvas(set_brush)(canvas_t* canvas, const color_t color, const f32 size);
void Canvas(update)(canvas_t* canvas, const mat4* projection);

#endif //CANVAS_H
