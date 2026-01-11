#pragma once
#include <defines.h>
#include <sprite.h>
#include <texture.h>
#include <event_system.h>

#ifndef BUTTON_H
#define BUTTON_H

typedef struct button {
    comp_header_t header;

    color_t bg;
    void* parent;

    sprite_t* obj;
    texture_t* tex;
} button_t;

button_t* new_button(void* parent, const color_t bg, const u32 x, const u32 y, const u32 width, const u32 height);
void del_button(button_t* btn);
void bind_button(const button_t* btn);
void update_button(const button_t* btn);

#endif //BUTTON_H
