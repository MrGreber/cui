#pragma once
#include <defines.h>
#include <math-utils.h>

#ifndef CAMERA_H
#define CAMERA_H

#define angle16 u16

typedef struct camera {
    vec2 position; /**< Camera position in 2D space */
    f32 zoom;      /**< Zoom level (scale factor) */
    angle16 roll;      /**< Rotation angle in radians */
    u16 keys;
} camera_t;

#define Camera(func) __camera_##func
__forceinline camera_t Camera(new)() {
    camera_t camera = { 0 };
    camera.zoom = 1.0f;
    return camera;
}

__forceinline void Camera(reset)(camera_t* camera) {
    camera->roll = 0;
    camera->zoom = 1.0f;
    camera->position = (vec2){ 0 };
}

__forceinline f32 Camera(from_angle16)(const u16 n) {
    return (f32)n * 0.0000152587890625f * 360.0f;
}
__forceinline u16 Camera(to_angle16)(const f32 n) {
    return (u16)(n * 65536.0f * 0.0027777777777777f);
}

#endif // CAMERA_H
