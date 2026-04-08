#pragma once
#include <defines.h>
#include <math-utils.h>

#ifndef CAMERA_H
#define CAMERA_H

typedef struct camera {
    vec2 position; /**< Camera position in 2D space */
    f32 zoom;      /**< Zoom level (scale factor) */
    f32 roll;      /**< Rotation angle in radians */
    u16 keys;
} camera_t;

#define Camera(func) __camera_##func
__forceinline camera_t Camera(new)() {
    camera_t camera = { 0 };
    camera.roll = 0.0f;
    camera.zoom = 1.0f;
    camera.position = (vec2){ 0 };
    return camera;
}

__forceinline void Camera(reset)(camera_t* camera) {
    camera->roll = 0.0f;
    camera->zoom = 1.0f;
    camera->position = (vec2){ 0 };
}

#endif // CAMERA_H
