#pragma once

#ifndef PANEL_H
#define PANEL_H
#include <utils.h>
#include <sprite.h>



typedef struct panel {
    component_header header;

    void* parent;

    sprite_t* sprite;
    texture_t* tex;
} panel_t;



#endif //PANEL_H
