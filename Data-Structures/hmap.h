#pragma once

#ifndef HMAP_H
#define HMAP_H
#include <defines.h>

typedef struct key_value_pair {
    u8 init;

    struct {
        u64 size;
        uptr ptr;
    } key;
    struct {
        u64 size;
        uptr ptr;
    } value;

    struct key_value_pair* next;
} kvp_t;

#define cast_u64(X) (*(u64*)(X))
__forceinline kvp_t new_kvp(const uptr key, const u64 ksize, const bool kheap, const uptr value, const u64 vsize, const bool vheap) {
    return (kvp_t){
        .key = {.size = ksize, .ptr = key},
        .value = {.size = vsize, .ptr = value},
        .init = (kheap << 1) | (vheap << 2)
    };
}
void print_kvp(const kvp_t* pair);

typedef u64 (*hash_function)(void*, u64);

typedef struct hashmap {
    u64 count;
    kvp_t* elem;
    hash_function func;

    struct {
        u64 count;
        u64 capacity;
        kvp_t* elem;
    } collisions;
} hmap_t;

hmap_t* new_hmap(u64 capacity, hash_function func);
void del_hmap(hmap_t* map);

kvp_t* search_hmap(hmap_t* map, uptr key, const u64 size);
bool insert_hmap(hmap_t* map, const kvp_t* src);
void print_hmap(hmap_t* map, const u8 indent);


#endif //HMAP_H
