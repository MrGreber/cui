#pragma once

#ifndef FRAME_H
#define FRAME_H
#include <defines.h>
#include <utils.h>
#include <event_system.h>
#include <stopwatch.h>
#include <shader/types.h>
#include <geometry/types.h>

// Todo: add a cache for fonts, maybe even separate this struct to a different struct well idk
struct frame_cache {
    uniform_hashmap_t uniforms;
    shader_t shaders[__SHADER_TAG_COUNT__];
    struct {
        static_mesh_t data[__MESH_TAG_COUNT__];
        elem_buf_t eb;
    } static_meshes;
    struct {
        dynamic_mesh_t* data;
        u16 count;
        u16 capacity;
    } dynamic_meshes;

    mat4 projection;
    // struct {
    //
    // } fonts;
};

typedef struct frame {
    comp_header_t header;
    char* title;

    color_t bg;
    void* ctx;        /**< GLFW window/context pointer */
    void* cursor;
    stopwatch_t stopwatch;

    comp_t focused;
    comp_t hovered;
    comp_t captured;
    struct frame_cache cache;
} frame_t;

#define Frame(func) __frame_##func
frame_t* Frame(new)(const color_t bg, const u32 width, const u32 height, const char* title);
void Frame(del)(frame_t* frame);

void Frame(update)(frame_t* frame);

void Frame(set_position)(frame_t* frame, const u16 x, const u16 y);
#define FRAME_HIDE 1
void Frame(set_flag)(frame_t* frame, const u8 field);
#endif // FRAME_H
