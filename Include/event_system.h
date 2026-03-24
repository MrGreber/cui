#pragma once

#ifndef EVENT_H
#define EVENT_H
#include <defines.h>
#include <utils.h>

typedef enum callback_type {
    MOUSE_CALLBACK,
    SCROLL_CALLBACK,
    KEYBOARD_CALLBACK
} callback_type;

typedef void (*callback)(void*);

typedef enum component_tag {
    FRAME_COMPONENT,
    PANEL_COMPONENT,
    BUTTON_COMPONENT,
    EDIT_COMPONENT,
    CANVAS_COMPONENT,
    __COMPONENT_TAG_COUNT__
} comp_tag;

typedef enum background_type {
    BG_NONE,
    BG_COLOR,
    BG_IMAGE,
    BG_GRADIENT
} bg_type_t;

typedef struct bounding_box {
    i32 x, y;
    u32 width, height;
} bounding_box;

typedef struct style {
    u8 init;

    u64 mode;
    struct {
        union {
            color_t color;
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
    u8 focus;
    bounding_box box;
    bounding_box content_box;

    callback keyboard;
    callback mouse;
    callback scroll;
    callback resize;
    void* components;
} comp_header_t;

typedef struct component {
    void* inst;
    comp_tag tag;
} comp_t;

typedef struct component_node {
    struct component_node* root;

    comp_t component;

    u64 capacity;
    u64 count;
    struct component_node** nodes;
} comp_node_t;


#define get_header(COMP) ((comp_header_t*)(COMP))
#define bounded(mx, my, x, y, w, h) (((mx) >= (x) && (mx) < ((x) + (w))) && ((my) >= (y) && (my) < ((y) + (h))))
__forceinline void* get_root(void* comp) {
    return ((comp_node_t*)((comp_header_t*)comp)->components)->root->component.inst;
}

comp_node_t* new_comp_node(void* data, const comp_tag tag);
void del_comp_node(comp_node_t* root);
bool push_comp_node(comp_node_t* root, void* val, const comp_tag tag);
void print_comp_node(comp_node_t* root, u64 indent);

void dispatch_event(const comp_node_t* node, event_t* event);

#endif //EVENT_H
