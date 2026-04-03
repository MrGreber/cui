#include <camera.h>
#include <utils.h>
#include <memio.h>

#include <stdio.h>

camera_t* new_camera() {
    buf_t buffer = {
        .size = sizeof(camera_t),
        .tag = MEMTAG_CAMERA,
    };
    if (!Buffer(new)(&buffer, true)) return NULL;

    camera_t* cam = buffer.ptr;
    cam->roll = 0.0f;
    cam->zoom = 1.0f;
    cam->position = (vec2){ 0 };
    return cam;
}

void del_camera(camera_t* cam) {
    if (!cam) return;
    Buffer(del)(&(buf_t){.size = sizeof(camera_t), .tag = MEMTAG_CAMERA, .ptr = cam});
}

void reset_camera(camera_t* cam) {
    cam->roll = 0.0f;
    cam->zoom = 1.0f;
    cam->position = (vec2){ 0 };
}