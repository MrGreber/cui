#include <array.h>
#include <mem.h>
#include <log.h>

#include <corecrt_memcpy_s.h>

array_t* new_array(u64 capacity, const u64 size, const element_free free) {
    if (!size || !free) {
        logError("new_array - Invalid parameters, size or free.");
        return NULL;
    }
    if (!capacity) capacity = 32;

    buf_t buffer = {
        .size = sizeof(array_t),
        .tag =  MEMTAG_ARRAY
    };
    if (!new_buf(&buffer, false)) return NULL;

    array_t* array = buffer.ptr;
    array->capacity = capacity;
    array->size = size;
    array->count = 0;

    buffer = (buf_t){
        .ptr = NULL,
        .size = size * capacity,
        .tag = MEMTAG_POINTER
    };
    if (!new_buf(&buffer, false)) {
        logError("new_array - Failed to allocate array.");
        del_buf(&(buf_t){.ptr = array, .size = sizeof(array_t), .tag = MEMTAG_ARRAY});
        return NULL;
    }
    array->elements = buffer.ptr;
    array->free = free;

    return array;
}

void del_array(array_t* array) {
    if (!array) return;

    if (array->free) {
        for (u64 i = 0; i < array->capacity; i++) {
            void* ptr = (byte*)array->elements + array->size * i;
            array->free(ptr);
        }
    }
    del_buf(&(buf_t){.ptr = array->elements, .size = array->size * array->capacity, .tag = MEMTAG_POINTER});
    del_buf(&(buf_t){.ptr = array, .size = sizeof(array_t), .tag = MEMTAG_ARRAY});
}

static bool __resize_array(array_t* array) {
    if (array->capacity == UINT64_MAX) {
        logError("__resize_app_vars - Failed to resize vars app, vars reached max size %d.", UINT16_MAX);
        return false;
    }

    buf_t buffer = {
        .ptr = array->elements,
        .size = array->size * array->capacity,
        .tag = MEMTAG_POINTER
    };
    const u64 new_cap = array->capacity << 1;
    if (!renew_buf(&buffer, sizeof(void*) * new_cap)) return false;
    array->capacity = new_cap;

    return true;
}

void push_array(array_t* array, const void* element) {
    if (!array) return;
    if (array->capacity <= array->count && !__resize_array(array)) goto cleanup;

    void* address = (byte*)array->elements + array->size * array->count++;
    memcpy_s(address, array->size, element, array->size);

    return;
cleanup:
    logError("push_array - Failed to resize array.");
}
void* get_array(array_t* array, const u64 index) {
    if (!array) return NULL;
    if(index >= array->count) {
        logError("get_array - Failed to get array element, index %d out of bound.", index);
        return NULL;
    }

    return (byte*)array->elements + array->size * index;
}