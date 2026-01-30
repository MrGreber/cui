#include <hmap.h>
#include <mem.h>
#include <log.h>


#include <memory.h>
#include <stdio.h>
#define DEFAULT_CAPACITY 128

static u64 __FNV_1A(void* data, u64 size) {
    u64 hash = 1469598103934665603ULL;
    const u8* ptr = data;

    for (u64 i = 0; i < size; i++) {
        hash ^= ptr[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

hmap_t* new_hmap(u64 capacity, hash_function func) {
    if (!capacity) capacity = DEFAULT_CAPACITY;
    if (!func) func = __FNV_1A;

    buf_t buffer = {
        .size = sizeof(hmap_t),
        .tag = MEMTAG_HASHMAP
    };
    if (!new_buf(&buffer, true)) {
        logError("new_hmap - Failed to allocate hashmap.");
        goto cleanup;
    }
    hmap_t* map = buffer.ptr;

    buffer = (buf_t){
        .size = sizeof(kvp_t) * DEFAULT_CAPACITY,
        .tag = MEMTAG_KEY_VALUE_PAIR
    };
    if (!new_buf(&buffer, true)) {
        logError("new_hmap - Failed to allocate hashmap element array.");
        goto cleanup;
    }
    map->elem = buffer.ptr;

    buffer = (buf_t){
        .size = sizeof(kvp_t) * DEFAULT_CAPACITY,
        .tag = MEMTAG_KEY_VALUE_PAIR
    };
    if (!new_buf(&buffer, true)) {
        logError("new_hmap - Failed to allocate hashmap element array.");
        goto cleanup;
    }
    map->collisions.elem = buffer.ptr;
    map->collisions.capacity = DEFAULT_CAPACITY;
    map->func = func;

    return map;
cleanup:
    if (map->elem) del_buf(&(buf_t){.ptr = map->elem, .size = sizeof(kvp_t) * DEFAULT_CAPACITY, .tag = MEMTAG_KEY_VALUE_PAIR});
    if (map->collisions.elem) del_buf(&(buf_t){.ptr = map->collisions.elem, .size = sizeof(kvp_t) * DEFAULT_CAPACITY, .tag = MEMTAG_KEY_VALUE_PAIR});
    del_buf(&(buf_t){.ptr = map, .size = sizeof(hmap_t), .tag = MEMTAG_HASHMAP});
    return NULL;
}
void del_hmap(hmap_t* map) {
    if (!map) return;
    del_buf(&(buf_t){.ptr = map->elem, .size = sizeof(kvp_t) * DEFAULT_CAPACITY, .tag = MEMTAG_KEY_VALUE_PAIR});
    del_buf(&(buf_t){.ptr = map->collisions.elem, .size = sizeof(kvp_t) * DEFAULT_CAPACITY, .tag = MEMTAG_KEY_VALUE_PAIR});
    del_buf(&(buf_t){.ptr = map, .size = sizeof(hmap_t), .tag = MEMTAG_HASHMAP});
}

kvp_t* search_hmap(hmap_t* map, uptr key, const u64 size) {
    if (!map || !key || !size) return NULL;

    u8* ptr;
    if (size <= sizeof(void*)) ptr = (u8*)&key;
    else ptr = (u8*)key;

    const u64 index = map->func(ptr, size) & (map->collisions.capacity - 1);

    kvp_t* pair = &map->elem[index];
    u8* pair_ptr = size <= sizeof(void*) ? (u8*)&pair->key.ptr : (u8*)pair->key.ptr;
    while (pair->key.size != size || pair_ptr[0] != ptr[0] || memcmp(ptr, pair_ptr, size)) {
        pair = pair->next;
        pair_ptr = size <= sizeof(void*) ? (u8*)&pair->key.ptr : (u8*)pair->key.ptr;
    }

    return pair;
}

bool __resize_hashmap(hmap_t* map) {
    if (map->collisions.capacity >= UINT64_MAX) {
        logError("__resize_hashmap - Failed to resize hashmap, hashmap collision array reached max size %d.", UINT64_MAX);
        return false;
    }

    const u64 new_capacity = map->collisions.capacity << 1;
    buf_t buffer = {
        .ptr = map->collisions.elem,
        .size = map->collisions.capacity * sizeof(kvp_t),
        .tag = MEMTAG_KEY_VALUE_PAIR
    };
    if (!renew_buf(&buffer, new_capacity * sizeof(kvp_t))) return false;

    map->collisions.elem =  buffer.ptr;
    map->collisions.capacity = new_capacity;

    return true;
}

bool insert_hmap(hmap_t* map, const kvp_t* src) {
    if (!map || !src) return false;
    if (4 * map->collisions.count >= 3 * map->collisions.capacity && !__resize_hashmap(map)) return false;

    u8* ptr;
    if (src->key.size <= sizeof(uptr)) ptr = (u8*)&src->key.ptr;
    else ptr = (u8*)src->key.ptr;

    const u64 index = map->func(ptr, src->key.size) & (map->collisions.capacity - 1);
    kvp_t* dst = &map->elem[index];
    u8* pair_ptr = dst->key.size <= sizeof(void*) ? (u8*)&dst->key.ptr : (u8*)dst->key.ptr;
    if (
        dst->key.size == src->key.size &&
        pair_ptr[0] == ptr[0] &&
        !memcmp(ptr, pair_ptr, src->key.size)
    ) dst->value = src->value;

    else if (!dst->init) {
        memcpy(dst, src, sizeof(kvp_t));
        dst->init |= 1;
    }
    else {
        kvp_t* cur = dst;
        while (
            cur->next &&
            (
                cur->key.size != src->key.size ||
                pair_ptr[0] != ptr[0] ||
                memcmp(ptr, pair_ptr, src->key.size)
            )
        ) {
            cur = cur->next;
            pair_ptr = dst->key.size <= sizeof(void*) ? (u8*)&dst->key.ptr : (u8*)dst->key.ptr;
        }

        if (
            cur->key.size != src->key.size ||
            pair_ptr[0] != ptr[0] ||
            memcmp(ptr, pair_ptr, src->key.size)
        ) {
            kvp_t* next = &map->collisions.elem[map->collisions.count++];
            memcpy(next, src, sizeof(kvp_t));
            next->init |= 1;
            cur->next = next;
        }
        else cur->value = src->value;

    }

    return true;
}

void print_kvp(const kvp_t* pair) {
    printf("%p[%lld]: %p[%lld]\n", (void*)pair->key.ptr, pair->key.size, (void*)pair->value.ptr, pair->value.size);
}

void print_hmap(hmap_t* map, const u8 indent) {
    if (!map) return;
    for (u64 i = 0; i < DEFAULT_CAPACITY; i++) {
        const kvp_t* pair = &map->elem[i];
        if (pair->init) print_kvp(pair);
    }
    for (u64 i = 0; i < map->collisions.count; i++) {
        const kvp_t* pair = &map->collisions.elem[i];
        if (pair->init) print_kvp(pair);
    }
}
