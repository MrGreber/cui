#pragma once

#ifndef EVENTS_H
#define EVENTS_H
#include <defines.h>

typedef enum event_type {
    __MOUSE_EVENT__,
    __SCROLL_EVENT__,
    __KEYBOARD_EVENT__,
    __RESIZE_EVENT__
} event_type_t;

typedef struct mouse_callback_parameter {
    void* instance;
    i32 x;
    i32 y;
    i32 button;
    i32 action;
    i32 mods;
} mouse_cb_param;

typedef struct keyboard_callback_parameter {
    void* instance;
    i32 key;
    i32 scancode;
    i32 action;
    i32 modes;
} keyboard_cb_param;

typedef struct scroll_callback_parameter {
    void* instance;
    f64 delta;
} scroll_cb_param;

typedef struct resize_callback_parameter {
    void* instance;
    i32 width;
    i32 height;
} resize_cb_param;

typedef struct event {
    union {
        scroll_cb_param scroll;
        mouse_cb_param mouse;
        keyboard_cb_param keyboard;
        resize_cb_param resize;
    } param;

    event_type_t type;
} event_t;

#endif // EVENTS_H
