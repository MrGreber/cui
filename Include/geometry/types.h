#pragma once

#ifndef TYPES_H
#define TYPES_H
#include <defines.h>
#include <math-utils.h>

typedef union vertex_buffer {
    u32 id;
    struct {
        u32 gl_id : 31;
        u32 dynamic : 1;
    };
} vert_buf_t;

typedef struct element_buffer {
    u32 id;
} elem_buf_t;

typedef struct vertex_array_element {
    u32 count;
    union {
        i32 type;
        struct {
            u32 gl_type : 31;
            u32 normalized : 1;
        };
    };
} vert_elem_t;

static_assert(sizeof(vert_buf_t) == 4);
static_assert(sizeof(vert_elem_t) == 8);

typedef struct vertex_array {
    u32 id;
    u32 stride;
    u32 count;
    u32 capacity;
    vert_elem_t* elem;
} vert_array_t;

typedef enum mesh_tag {
    RECT_MESH = 0,
    __MESH_TAG_COUNT__,
    DYNAMIC_MESH
} mesh_tag_t;

typedef struct mesh_metadata {
    mesh_tag_t tag;
    vert_buf_t vb;
    vert_array_t* va;
} mesh_metadata_t, static_mesh_t;

typedef struct dynamic_mesh {
    mesh_metadata_t metadata;
    elem_buf_t eb;
    u64 count;
    u64 capacity;
    vec4* vertices;
} dynamic_mesh_t, mesh_t;

#endif // TYPES_H