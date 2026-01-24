#pragma once

#ifndef PANEL_H
#define PANEL_H
#include <utils.h>
#include <sprite.h>

typedef struct panel {
    comp_header_t header;
    style_group_t styles;

    void* parent;
    struct {
        mat4 model;
        u8 init;
    } transform;

    sprite_t* sprite;
} panel_t;


panel_t* new_panel(void* parent, style_group_t* group, const bounding_box* box);
void del_panel(panel_t* panel);
void bind_panel(const panel_t* panel);
void update_panel(panel_t* panel, const mat4* projection, const f32 angle);

#endif //PANEL_H
