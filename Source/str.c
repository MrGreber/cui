#include <str.h>
#include <memio.h>
#include <error.h>

#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <utils.h>


#define DEFAULT_CAPACITY 128
static u64 private(static_length)(char_t* data) {
    const u64 start = (u64)data;
    while(*++data) {}
    return (u64)data - start;
}
static bool private(resize_string)(str_t* src) {
    if (src->capacity == UINT64_MAX) {
        logError(ERR_HEAP_REALLOC, "vars reached max size %d.", UINT16_MAX);
        return false;
    }

    buf_t buffer = {
        .ptr = src->data,
        .size = sizeof(char_t) * src->capacity,
        .tag = MEMTAG_BYTE
    };
    const u64 new_cap = src->capacity << 1;
    if (!Buffer(renew)(&buffer, sizeof(char_t) * new_cap)) return false;
    src->data = buffer.ptr;
    src->capacity = new_cap;
    return true;
}

str_t* String(new)(char_t* data, u64 length) {
    u64 cap = 0;
    if (!data) return NULL;
    if (!length && !data[0]) cap = DEFAULT_CAPACITY;
    else {
        length = private(static_length)(data);
        cap = __closest_pow2(length);
    }

    buf_t buffer = {
        .size = sizeof(str_t),
        .tag = MEMTAG_STRING
    };
    if (!Buffer(new)(&buffer, false)) {
        logError(ERR_HEAP_ALLOC, "Failed to allocate string.");
        return NULL;
    }
    str_t* string = buffer.ptr;
    string->capacity = cap;
    string->length = length;

    const u64 size = sizeof(char_t) * string->capacity;
    buffer = (buf_t){
        .size = size,
        .tag = MEMTAG_BYTE
    };
    if (!Buffer(new)(&buffer, true)) {
        Buffer(del)(&(buf_t){.ptr = string, .size = sizeof(str_t), .tag = MEMTAG_BYTE});
        logError(ERR_HEAP_ALLOC, "Failed to allocate string buffer.");
        return NULL;
    }
    string->data = buffer.ptr;
    if (data[0]) {
        while (string->capacity <= length && !private(resize_string)(string));
        memcpy(string->data, data, (length >= string->capacity ? string->capacity : length) * sizeof(char_t));
    }
    return string;
}

void String(del)(str_t* src) {
    if (!src) return;
    Buffer(del)(&(buf_t){.ptr = src->data, .size = sizeof(char_t) * src->capacity, .tag = MEMTAG_BYTE});
    Buffer(del)(&(buf_t){.ptr = src, .size = sizeof(str_t), .tag = MEMTAG_STRING});
}


bool String(set)(str_t* dst, char_t* src, const u64 length) {
    if (!dst || !src || !length) return false;
    while (dst->capacity <= length && !private(resize_string)(dst));
    memcpy(dst->data, src, (length >= dst->capacity ? dst->capacity : length) * sizeof(char_t));
    dst->length = length;
    return true;
}

bool String(popC)(str_t* src, const u64 index) {
    if (!src || index >= src->length) return false;

    char_t* ptr = src->data;
    memmove(&ptr[index], &ptr[index + 1], (src->length - index - 1) * sizeof(char_t));
    src->length--;
    return true;
}
bool String(del_sub)(str_t* src, const u64 start, const u64 end) {
    if (!src || start >= src->length || end >= src->length) return false;

    char_t* ptr = src->data;
    const u64 delta = end + 1 - start;
    src->length -= delta;

    if (!src->length) return true;
    for (u64 i = start; i < end + 1; i++) ptr[i] = ptr[i + delta];
    return true;
}
str_t* String(get_sub)(str_t* src, const u64 start, const u64 end) {
    if (!src || start >= src->length || end >= src->length) return NULL;
    return String(new)(src->data + start * sizeof(char_t), end - start);
}
bool push_char(str_t* src, const char_t c) {
    if (!src) return false;
    if (src->capacity <= src->length && !private(resize_string)(src)) goto cleanup;

    src->data[src->length++] = c;
    return true;
cleanup:
    logError(ERR_HEAP_REALLOC, "Failed to resize string buffer.");
    return false;
}
bool String(insertC)(str_t* src, const u64 index, const char_t c) {
    if (!src || index > src->length) return false;
    if (src->capacity <= src->length && !private(resize_string)(src)) goto cleanup;

    char_t* ptr = src->data;
    memmove(&ptr[index + 1], &ptr[index], (src->length - index) * sizeof(char_t));

    src->data[index] = c;
    src->length++;
    return true;
cleanup:
    logError(ERR_HEAP_REALLOC, "Failed to resize string buffer.");
    return false;
}
bool String(concat)(str_t* dst, str_t* src) {
    if (!dst || !src) return false;

    const u64 index = dst->length;
    dst->length += src->length;
    while (dst->capacity <= dst->length && !private(resize_string)(dst));
    memcpy(dst->data, src->data, (dst->capacity <= dst->length ? dst->capacity : dst->length) * sizeof(char_t));
    memcpy(dst->data + index, src->data, src->length);

    return true;
}
u64 String(findC)(str_t* src, const u64 start, const char_t c) {
    if (!src) return 0;
    u64 i = start;
    for (; src->data[i] != c && i < src->length; i++) {}
    return i;
}
u64 String(rfindC)(str_t* src, const u64 start, const char_t c) {
    if (!src) return 0;
    u64 i = start;
    for (; src->data[i] != c && (i64)i > 0; i--);
    if (i) i++;
    return i;
}

void String(print)(const str_t* src, const bool new_line) {
    char_t* ptr = src->data;
    for (u64 i = 0; i < src->length; i++) {
        // Todo: currently just utf-8, NO UNICODE!!!!
        #if !UNICODE
        #else
        fputc(ptr[i], stdout);
        #endif
    }
    if (new_line) fputc('\n', stdout);
}
