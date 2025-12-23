#pragma once
#include <defines.h>
#include <camera.h>
#include <sprite.h>
#include <texture.h>
#include <event_system.h>

#ifndef CANVAS_H
#define CANVAS_H

/**
 * @enum brush_type
 * @brief Specifies the type of brush for drawing on the canvas.
 */
typedef enum brush_type {
    BR_PX1, /**< 1-pixel brush, for precise drawing */
    BR_ERS  /**< Eraser brush, removes pixels from the canvas */
} brush_type;

/**
 * @struct brush
 * @brief Represents a brush used to draw on a canvas.
 */
typedef struct brush {
    color_t color;     /**< Color of the brush */
    brush_type type;   /**< Type of the brush (e.g., pixel, eraser) */
    f32 size;          /**< Brush size in pixels */
} brush_t;

/**
 * @struct canvas
 * @brief Holds OpenGL drawing context, pixel buffer, and canvas properties.
 */
typedef struct canvas {
    comp_header header;

    void* parent;       /**< Optional parent object (can be NULL) */

    brush_t brush;      /**< Current brush settings for drawing */

    camera_t* camera;   /**< Pointer to canvas camera for transformations */
    sprite_t* obj;      /**< Pointer to canvas object */
    texture_t* tex;     /**< Pointer to texture used for rendering the canvas */
} canvas_t;




/**
 * @brief Create a new canvas.
 * @param parent Optional parent object
 * @param bg Background color of the canvas
 * @param x X position in pixels
 * @param y Y position in pixels
 * @param width Width of the canvas in pixels
 * @param height Height of the canvas in pixels
 * @return Pointer to the allocated canvas_t, or NULL on failure
 */
canvas_t* new_canvas(void* parent, const color_t bg, const u32 x, const u32 y, const u32 width, const u32 height);

/**
 * @brief Delete a canvas and free its resources.
 * @param can Pointer to the canvas to delete
 */
void del_canvas(canvas_t* can);

/**
 * @brief Bind the canvas for OpenGL rendering.
 * @param can Pointer to the canvas to bind
 */
void bind_canvas(const canvas_t* can);

/**
 * @brief Update the OpenGL texture with the current pixel buffer contents.
 * @param can Pointer to the canvas
 */
void update_canvas(const canvas_t* can);

/**
 * @brief Clear or flush the canvas buffer.
 * @param can Pointer to the canvas
 */
void flush_canvas(const canvas_t* can);

/**
 * @brief Resize the canvas.
 * @param can Pointer to the canvas
 * @param x New X position
 * @param y New Y position
 * @param width New canvas width
 * @param height New canvas height
 * @param frame_width Frame width for reference
 * @param frame_height Frame height for reference
 * @return true if successful, false otherwise
 */
bool resize_canvas(canvas_t* can, const u32 x, const u32 y, const u32 width, const u32 height, const u32 frame_width, const u32 frame_height);



/**
 * @brief Draw a single pixel on the canvas.
 * @param color Color of the pixel
 * @param x X coordinate of the pixel
 * @param y Y coordinate of the pixel
 */
void draw_pixel(const color_t color, const int x, const int y);

/**
 * @brief Draw a line between two points on the canvas.
 * @param color Line color
 * @param x0 Starting X coordinate
 * @param y0 Starting Y coordinate
 * @param x1 Ending X coordinate
 * @param y1 Ending Y coordinate
 */
void draw_line(const color_t color, int x0, int y0, const int x1, int y1);

/**
 * @brief Set the current brush for the canvas.
 * @param can Pointer to the canvas
 * @param color Brush color
 * @param type Brush type (PX1, ERS, etc.)
 */
void set_canvas_brush(canvas_t* can, const color_t color, const brush_type type);

__forceinline void set_canvas_callbacks(canvas_t* can, const callback_type type, void* callback) {
    switch (type) {
        case MOUSE_CALLBACK: {can->header.mouse = callback; break;}
        case KEYBOARD_CALLBACK: {can->header.keyboard = callback; break;}
        case SCROLL_CALLBACK: {can->header.scroll = callback; break;}
    }
}

#endif // CANVAS_H
