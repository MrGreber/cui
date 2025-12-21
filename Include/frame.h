#pragma once
#include <defines.h>
#include <utils.h>
#include <event_system.h>

#ifndef FRAME_H
#define FRAME_H

/**
 * @struct frame
 * @brief Represents a rendering frame or window.
 *
 * Holds the window/context pointer, input callbacks, dimensions, and background color.
 */
typedef struct frame {
    component_header header;

    void* glfw_ctx;        /**< GLFW window/context pointer */

    component_t focused;
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

#endif // FRAME_H
