#pragma once

#ifndef TEXTURE_H
#define TEXTURE_H
#include <defines.h>
#include <utils.h>
#include <event_system.h>

/**
 * @struct texture
 * @brief Represents an OpenGL texture and its optional framebuffer.
 *
 * Contains the OpenGL texture ID and a framebuffer ID if used as a render target.
 */
typedef struct texture {
    u32 id;     /**< OpenGL texture ID */
    u32 fb_id;  /**< OpenGL framebuffer ID (0 if not used) */
} texture_t;

/**
 * @brief Create a new texture with the specified dimensions.
 * @param data
 * @param width Texture width in pixels
 * @param height Texture height in pixels
 * @return Pointer to the allocated texture_t, or NULL on failure
 */
texture_t* new_texture(const color_t* data, const u32 width, const u32 height);

/**
 * @brief Delete a texture and free its resources.
 * @param tex Pointer to the texture to delete
 */
void del_texture(texture_t* tex);

void bind_texture(const texture_t* tex);
void unbind_texture();

void flush_texture(const texture_t* tex, const color_t bg);

void set_texture_pixel(const texture_t* tex, const color_t color, const i32 x, const i32 y);
void draw_texture_line(const texture_t* tex, const color_t color, i32 x0, i32 y0, const i32 x1, const i32 y1);

color_t* load_texture(const char* path, u32* width, u32* height);
bool gen_texture(struct texture** out, const bounding_box* box, const style_t* style);

#endif // TEXTURE_H

