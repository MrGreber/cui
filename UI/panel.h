#pragma once

#ifndef PANEL_H
#define PANEL_H
#include <sprite.h>

#define EMBEDDED_POPUP U64(0x1)
#define WINDOW_POPUP (EMBEDDED_POPUP << 1)
#define STATIC_POPUP (WINDOW_POPUP << 1)
#define CAPTION (STATIC_POPUP << 1)
#define CLOSE_BUTTON (CAPTION << 1)

typedef struct panel {
    comp_header_t header;
    style_group_t styles;

    void* parent;
    struct {
        mat4 model;
        u8 init;
    } transform;

    sprite_t* sprite;
    struct {
        bool state;
        struct {
            i32 x, y;
        } prev;
    } drag;
} panel_t;

#define Panel(func) __panel_##func
panel_t* Panel(new)(void* parent, style_group_t* group, const bounding_box* box);
void Panel(del)(panel_t* panel);
void Panel(bind)(const panel_t* panel);
void Panel(update)(panel_t* panel);

#endif //PANEL_H
