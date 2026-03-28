#pragma once

#ifndef FRAME_H
#define FRAME_H
#include <defines.h>
#include <utils.h>
#include <event_system.h>
#include <stopwatch.h>
#include <shader/types.h>
#include <geometry/types.h>

typedef enum frame_flag{
    HIDE_FLAG
} frame_flag;

/**
 * @struct frame
 * @brief Represents a rendering frame or window.
 *
 * Holds the window/context pointer, input callbacks, dimensions, and background color.
 */
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

    struct {
        uniform_hashmap_t uniforms;
        shader_t shaders[__SHADER_TAG_COUNT__];
        static_mesh_t static_meshes[__MESH_TAG_COUNT__];
        struct {
            dynamic_mesh_t* data;
            u16 count;
            u16 capacity;
        } dynamic_meshes;
    } cache;
    byte flags;
} frame_t;

/**
 * @brief Create a new frame (window) with specified parameters.
 * @param bg Background color of the frame
 * @param width Frame width in pixels
 * @param height Frame height in pixels
 * @param title Window title
 * @return Pointer to the newly allocated frame_t, or NULL on failure
 */
frame_t* new_frame(const color_t bg, const u32 width, const u32 height, const char* title);
void del_frame(frame_t* frame);

void update_frame(frame_t* frame);

void set_frame_position(frame_t* frame, const u16 x, const u16 y);
void set_frame_flag(frame_t* frame, const frame_flag field);
#endif // FRAME_H
