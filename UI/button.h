#pragma once

#ifndef BUTTON_H
#define BUTTON_H
#include <utils.h>
#include <sprite.h>

typedef void (*button_callback)(void* vp_button);

typedef struct button {
    comp_header_t header;
    style_group_t styles;

    void* parent;
    void* user_data;
    button_callback on_click;

    sprite_t* sprite;
    texture_t* tex;
} button_t;

button_t* new_button(void* parent, style_group_t* group, const bounding_box* box);
void del_button(button_t* button);
void bind_button(const button_t* button);
void update_button(button_t* button, const mat4* projection, const f32 angle);
__forceinline void set_button_callback(button_t* button, const button_callback callback) {
    button->on_click = callback;
}
__forceinline void set_button_user_data(button_t* button, void* user_data) {
    button->user_data = user_data;
}

#endif //BUTTON_H
