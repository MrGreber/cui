#pragma once


#ifndef POPUP_H
#define POPUP_H
#include <utils.h>
#include <sprite.h>

typedef struct popup {
    comp_header_t header;
    style_group_t styles;

    void* parent;
    struct {
        mat4 model;
        u8 init;
    } transform;

    sprite_t* sprite;
} popup_t;

popup_t* new_popup(const color_t bg, const u32 width, const u32 height, const char* title);

#endif //POPUP_H
