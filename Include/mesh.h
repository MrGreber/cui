#pragma once

#ifndef MESH_H
#define MESH_H
#include <utils.h>
#include <geometry.h>

typedef struct mesh {
    u64 count;
    u64 capacity;
    vec4* vertices;

    vert_array* va;
    vert_buf* vb;
} mesh_t;


#endif //MESH_H
