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

    sprite_t* sprite;

    brush_t brush;
    struct {
        i16 x;
        i16 y;
    } prev;

    mat4 model;
    mat4 inv_model;
    camera_t camera;
} canvas_t;

#define Canvas(func) __canvas_##func
canvas_t* Canvas(new)(void* parent, const u32 width, const u32 height);
void Canvas(del)(canvas_t* canvas);
void Canvas(bind)(canvas_t* canvas);
void Canvas(set_brush)(canvas_t* canvas, const color_t color, const f32 size);
void Canvas(update)(canvas_t* canvas);

#endif //CANVAS_H
