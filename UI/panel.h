#pragma once

#ifndef PANEL_H
#define PANEL_H
#include <utils.h>
#include <sprite.h>

typedef struct panel {
    comp_header header;
    comp_style_set styles;

    void* parent;

    sprite_t* sprite;
    texture_t* tex;
} panel_t;


panel_t* new_panel(void* parent, const color_t bg, const bounding_box* box);
void del_panel(panel_t* panel);
void bind_panel(const panel_t* panel);
void update_panel(panel_t* panel, const mat4* projection, const f32 angle);

#endif //PANEL_H
