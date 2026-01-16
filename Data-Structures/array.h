#pragma once
#include <defines.h>

#ifndef ARRAY_H
#define ARRAY_H

typedef void (*element_free)(void*);
typedef struct array {
    u64 count;
    u64 capacity;
    u64 size;

    void* elements;
    element_free free;
} array_t;

array_t* new_array(u64 capacity, const u64 size, const element_free free);
void del_array(array_t* array);

void push_array(array_t* array, const void* element);
void* get_array(array_t* array, const u64 index);

#endif //ARRAY_H
