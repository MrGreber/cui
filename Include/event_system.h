#pragma once

#ifndef EVENT_H
#define EVENT_H
#include <defines.h>
#include <utils.h>

typedef enum background_type {
    BG_NONE,
    BG_TEST,
    BG_COLOR,
    BG_IMAGE,
    BG_LINEAR_GRADIENT,
    BG_RADIAL_GRADIENT,
    BG_MANDELBROT
} bg_type_t;

typedef struct bounding_box {
    i32 x, y;
    u32 width, height;
} bounding_box;

struct gradient_metadata {
    color_t* colors;
    f32* positions;
    u8 count;
};
typedef struct linear_gradient {
    struct gradient_metadata metadata;
    f32 angle;
} linear_grad_t;

typedef struct radial_gradient {
    struct gradient_metadata metadata;
    vec2 center;
    vec2 radii;
} radial_grad_t;

typedef struct style {
    struct {
        u64 mode: 63;
        u64 init: 1;
    };
    struct {
        union {
            color_t color;
            linear_grad_t* linear_gradient;
            radial_grad_t* radial_gradient;
            struct texture* texture;
            const char* image;
        };
        bg_type_t type;
        color_t mask;
    } background;
    struct {
        color_t color;
        u32 thickness;
        u32 radius;
    } border;
    struct {
        u32 left, right, top, bottom;
    } padding;
} style_t;

typedef struct style_group {
    style_t normal;
    style_t hover;
} style_group_t;

typedef enum callback_type {
    MOUSE_CALLBACK,
    SCROLL_CALLBACK,
    KEYBOARD_CALLBACK
} callback_type;

typedef void (*callback)(void*);

typedef enum component_tag {
    FRAME_COMPONENT,
    PANEL_COMPONENT,
    CAPTION_COMPONENT,
    BUTTON_COMPONENT,
    EDIT_COMPONENT,
    CANVAS_COMPONENT,
    __COMPONENT_TAG_COUNT__
} comp_tag;

typedef enum event_tag {
    __MOUSE_EVENT__,
    __SCROLL_EVENT__,
    __KEYBOARD_EVENT__,
    __RESIZE_EVENT__
} event_tag;

typedef struct mouse_callback_parameter {
    void* instance;
    f64 x;
    f64 y;
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

    event_tag tag;
} event_t;

typedef struct component_header {
    bounding_box box;
    bounding_box content_box;

    callback keyboard;
    callback mouse;
    callback scroll;
    callback resize;
    void* components;
    u8 focus;
} comp_header_t;

typedef struct component {
    void* instance;
    comp_tag tag;
} comp_t;

typedef struct component_node {
    struct component_node* root;

    comp_t component;

    u64 capacity;
    u64 count;
    struct component_node** nodes;
} comp_node_t;


__forceinline bool is_bounded(const bounding_box* box, const i32 x, const i32 y) {
    return
        (x >= box->x && x < box->x + box->width) &&
        (y >= box->y && y < box->y + box->height);
}
__forceinline comp_header_t* get_header(void* comp) {
    return (comp_header_t*)comp;
}
__forceinline style_group_t* get_styles(void* comp) {
    return (style_group_t*)((byte*)comp + sizeof(style_group_t));
}
__forceinline void* get_root(const void* comp) {
    comp_node_t* root = ((comp_node_t*)((comp_header_t*)comp)->components)->root;
    if (root == NULL) return (void*)comp;
    return root->component.instance;
}

#define Component(func) __component_##func
comp_node_t* Component(new_node)(void* data, const comp_tag tag);
void Component(del_node)(comp_node_t* root);
bool Component(push_node)(comp_node_t* root, void* val, const comp_tag tag);
void Component(print_node)(comp_node_t* root, u64 indent);

void dispatch_event(const comp_node_t* node, event_t* event);

#endif //EVENT_H
