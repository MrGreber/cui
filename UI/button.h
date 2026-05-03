#pragma once

#ifndef BUTTON_H
#define BUTTON_H
#include <utils.h>
#include <sprite.h>

typedef void (*button_callback)(void* vp_button);

typedef struct button {
    comp_header_t header;
    style_group_t styles;

    sprite_t* sprite;
    void* user_data;
    button_callback on_click;
    mat4 model;
} button_t;

#define Button(func) __button_##func
button_t* Button(new)(void* parent, style_group_t* group, const bounding_box* box);
void Button(del)(button_t* button);
void Button(bind)(const button_t* button);
void Button(update)(button_t* button);
__forceinline void Button(set_callback)(button_t* button, const button_callback callback) {
    button->on_click = callback;
}
__forceinline void Button(set_user_data)(button_t* button, void* user_data) {
    button->user_data = user_data;
}

#endif //BUTTON_H
