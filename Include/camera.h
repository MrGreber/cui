#pragma once
#include <defines.h>
#include <utils.h>
#include <math-utils.h>

#ifndef CAMERA_H
#define CAMERA_H

/**
 * @struct camera
 * @brief Represents a 2D camera for a canvas or scene.
 *
 * Holds position, zoom, and rotation data. Precomputes sine and cosine
 * for efficient transformations.
 */
typedef struct camera {
    vec2 position; /**< Camera position in 2D space */
    f32 zoom;      /**< Zoom level (scale factor) */
    f32 roll;      /**< Rotation angle in radians */
    u16 keys;
} camera_t;

#define Camera(func) __camera_##func
camera_t* new_camera();

/**
 * @brief Delete a camera and free its resources.
 * @param cam Pointer to the camera to delete
 */
void del_camera(camera_t* cam);

/**
 * @brief Reset camera parameters to default values.
 * Sets position to (0,0), zoom to 1, and roll to 0.
 * @param cam Pointer to the camera to reset
 */
void reset_camera(camera_t* cam);

#endif // CAMERA_H
