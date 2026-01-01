#pragma once

#ifndef OBJECT_H
#define OBJECT_H
#include <geometry.h>
#include <shader.h>
#include <texture.h>

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


/**
 * @brief Create a rectangle object for 2D rendering.
 * @param name
 * @return Pointer to the allocated rectangle object
 */
sprite_t* new_sprite(const char* name);

/**
 * @brief Delete an object and free its resources.
 * @param obj Pointer to the object to delete
 */
void del_sprite(sprite_t* obj);

/**
 * @brief Bind the object for rendering.
 * @param obj Pointer to the object to bind
 */
void bind_sprite(const sprite_t* obj);

/**
 * @brief Unbind any currently bound object.
 */
void unbind_sprite(void);

#endif // OBJECT_H
