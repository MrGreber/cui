#pragma once

#ifndef SHADER_H
#define SHADER_H
#include <defines.h>

typedef struct uniform {
    u64 hash;
    uptr address;
    i32 location;
} uniform_t;

typedef struct uniform_hashmap_entry {
    uniform_t uniform;
    u32 psl;
} entry_t;

typedef struct uniform_hashmap {
    entry_t* entries;
    u32 capacity;
    u32 count;
} uniform_hashmap_t;

typedef enum shader_tag{
    COMP_SHADER = 0,
    TEXT_SHADER,
    RECT_SHADER,
    __SHADER_TAG_COUNT__
} shader_tag_t;

typedef struct shader {
    u32 id;
    uniform_hashmap_t* uniforms;
} shader_t;

#endif // SHADER_H
