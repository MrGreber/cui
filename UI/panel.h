#pragma once

#ifndef PANEL_H
#define PANEL_H
#include <utils.h>
#include <sprite.h>

#define EMBEDDED_POPUP U64(0x1)
#define WINDOW_POPUP (EMBEDDED_POPUP << 1)
#define CAPTION (WINDOW_POPUP << 1)
#define CLOSE_BUTTON (CAPTION << 1)
#define MINIMIZE_BUTTON (CLOSE_BUTTON << 1)

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
void update_panel(panel_t* panel, const mat4* projection);

#endif //PANEL_H
