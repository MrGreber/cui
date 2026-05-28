#pragma once

#ifndef CANVAS_H
#define CANVAS_H
#include <defines.h>
#include <utils.h>
#include <sprite.h>
#include <camera.h>

typedef struct brush {
    color_t color[2];
    struct {
        u32 size: 16;
        u32 metadata: 15;
        u32 idx : 1;
    };
} brush_t;

typedef struct canvas {
    comp_header_t header;

    mat4 model;
    mat4 inv_model;
    camera_t camera;

    sprite_t* sprite;

    brush_t brush;
} canvas_t;

#define Canvas(func) __canvas_##func
canvas_t* Canvas(new)(void* parent, const u16 width, const u16 height);
void Canvas(del)(canvas_t* canvas);
void Canvas(bind)(canvas_t* canvas);
void Canvas(set_brush)(canvas_t* canvas, const u8 brush_index, const color_t color, const u16 size);
void Canvas(update)(canvas_t* canvas);

#endif //CANVAS_H
