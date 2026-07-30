#pragma once

#ifndef COMPONENT_SYSTEM_H
#define COMPONENT_SYSTEM_H
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
    i16 x, y;
    u16 width, height;
} bounding_box;

/**
 * @brief To use gradient metadata add in the style struct the count of colors in the gradient
 */
struct gradient_metadata {
    color_t* colors;
    f32* positions;
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
        i16 thickness;
        i16 radius;
    } border;
    // struct {
    //     i16 left, right, top, bottom;
    // } padding;
    struct {
        u64 modes: 47;
        u64 count: 16;
        u64 init: 1;
    };
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

typedef struct component_node comp_node_t;

typedef struct component_vtable {
    callback free;
    callback tick;
    callback mouse;
    callback scroll;
    callback resize;
    callback update;
    callback keyboard;
} comp_vtable_t;

typedef struct component_header {
    bounding_box box;
    bounding_box content_box;

    comp_vtable_t* vtable;

    comp_node_t* components;
    void* parent;
    struct {
        u8 focus: 1;
        u8 drag:  1;
        u8 hide:  1;
        u8 dirty_matrix: 1;
        u8 dirty: 2;
        u8 flags: 2;
    };
} comp_header_t;

typedef struct component {
    void* instance;
    comp_tag tag;
} comp_t;

typedef struct component_node {
    struct component_node* root;
    struct component_node** nodes;

    union {
        comp_t component;
        struct {
            void* instance;
            comp_tag tag;
            u16 capacity;
            u16 count;
        };
    };
} comp_node_t;


__forceinline bool is_bounded(const bounding_box* box, const i32 x, const i32 y) {
    return (x >= box->x && x < box->x + box->width) && (y >= box->y && y < box->y + box->height);
}
__forceinline bool is_intersected(const bounding_box* a, const bounding_box* b) {
    return (a->x < b->x + b->width && b->x < a->x + a->width) && (a->y < b->y + b->height && b->y < a->y + a->height);
}

__forceinline comp_header_t* get_header(void* comp) {
    return (comp_header_t*)comp;
}
__forceinline style_group_t* get_styles(void* comp) {
    return (style_group_t*)((byte*)comp + sizeof(comp_header_t));
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
void Component(print_node)(comp_node_t* root, const u64 indent);
void Component(update)(const comp_node_t* node);
#endif // COMPONENT_SYSTEM_H
