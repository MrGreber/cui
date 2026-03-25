#pragma once

#ifndef OBJECT_H
#define OBJECT_H
#include <geometry.h>
#include <shader/types.h>
#include <texture.h>
#include <frame.h>

/**
 * @struct sprite
 * @brief Represents a drawable object in OpenGL.
 *
 * Contains vertex arrays, vertex buffers, element buffers, and an associated shader.
 */
typedef struct sprite {
    vert_array* va;   /**< Pointer to the vertex array object */
    vert_buf* vb;     /**< Pointer to the vertex buffer */
    elem_buf* eb;     /**< Pointer to the element/index buffer */
    texture_t* tex;
    shader_t* shader; /**< Pointer to the shader used for rendering */
} sprite_t;

sprite_t* new_sprite(frame_t* frame, const shader_tag_t tag);

/**
 * @brief Delete an object and free its resources.
 * @param sprite Pointer to the object to delete
 */
void del_sprite(sprite_t* sprite);

/**
 * @brief Bind the object for rendering.
 * @param sprite Pointer to the object to bind
 */
void bind_sprite(const sprite_t* sprite);

/**
 * @brief Unbind any currently bound object.
 */
void unbind_sprite(void);

bool set_sprite_texture(sprite_t* sprite, const u32 width, const u32 height, style_t* style);

#endif // OBJECT_H
