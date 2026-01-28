#pragma once
#include <defines.h>
#include <utils.h>
#include <event_system.h>

#ifndef FRAME_H
#define FRAME_H

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
    byte flags;

    comp_t focused;
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

/**
 * @brief Delete a frame and free its resources.
 * @param frame Pointer to the frame to delete
 */
void del_frame(frame_t* frame);

/**
 * @brief Update the frame (swap buffers, poll events, etc.)
 * @param frame Pointer to the frame to update
 */
void update_frame(const frame_t* frame);

void set_frame_position(frame_t* frame, const u16 x, const u16 y);
void set_frame_flag(frame_t* frame, const frame_flag field);
#endif // FRAME_H
