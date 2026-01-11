#pragma once
#include <defines.h>
#include <utils.h>

#ifndef EVENT_H
#define EVENT_H


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

typedef struct component_node {
    struct component_node* root;

    comp_t component;

    u64 capacity;
    u64 count;
    struct component_node** nodes;
} comp_node_t;



comp_node_t* new_comp_node(void* data, const comp_tag tag);
void del_comp_node(comp_node_t* root);
bool push_comp_node(comp_node_t* root, void* val, const comp_tag tag);
void print_comp_node(comp_node_t* root);

void dispatch_event(const comp_node_t* node, event_t* event);


#endif //EVENT_H
