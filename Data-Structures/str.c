#include <str.h>
#include <mem.h>
#include <log.h>

#include <corecrt_memcpy_s.h>
#include <stdio.h>

static u64 __static_length(char_t* data) {
    const u64 start = (u64)data;
    while(*++data) {}
    return (u64)data - start;
}
static __forceinline u64 __closest_pow2(u64 n) {
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;
    return n;
}

str_t* new_str(char_t* data, u64 length) {
    u64 cap = 0;
    if (!data) return NULL;
    if (!length && !data[0]) cap = 128;
    else {
        length = __static_length(data);
        cap = __closest_pow2(length);
    }

    buf_t buffer = {
        .size = sizeof(str_t),
        .tag = MEMTAG_STRING
    };
    if (!new_buf(&buffer, false)) {
        logError("new_str - Failed to allocate string.");
        return NULL;
    }
    str_t* string = buffer.ptr;
    string->capacity = cap;
    string->length = length;

    const u64 size = sizeof(char_t) * string->capacity;
    buffer = (buf_t){
        .ptr = NULL,
        .size = size,
        .tag = MEMTAG_BYTE
    };
    if (!new_buf(&buffer, true)) {
        del_buf(&(buf_t){.ptr = string, .size = sizeof(str_t), .tag = MEMTAG_BYTE});
        logError("new_str - Failed to allocate string buffer.");
        return NULL;
    }
    string->data = buffer.ptr;
    if (data[0]) memcpy_s(string->data, size, data, sizeof(char_t) * length);
    return string;
}
void del_str(str_t* src) {
    if (!src) return;
    del_buf(&(buf_t){.ptr = src->data, .size = sizeof(char_t) * src->capacity, .tag = MEMTAG_BYTE});
    del_buf(&(buf_t){.ptr = src, .size = sizeof(str_t), .tag = MEMTAG_STRING});
}

static bool __resize_string(str_t* src) {
    if (src->capacity == UINT64_MAX) {
        logError("__resize_app_vars - Failed to resize vars app, vars reached max size %d.", UINT16_MAX);
        return false;
    }

    buf_t buffer = {
        .ptr = src->data,
        .size = sizeof(char_t) * src->capacity,
        .tag = MEMTAG_BYTE
    };
    const u64 new_cap = src->capacity << 1;
    if (!renew_buf(&buffer, sizeof(char_t) * new_cap)) return false;
    src->data = buffer.ptr;
    src->capacity = new_cap;
    return true;
}
bool pop_char(str_t* src, const u64 index) {
    if (!src || index >= src->length) return false;

    char_t* ptr = src->data;
    memmove(&ptr[index], &ptr[index + 1], (src->length - index - 1) * sizeof(char_t));
    src->length--;
    return true;
}
bool del_substr(str_t* src, const u64 start, const u64 end) {
    if (!src || start >= src->length || end >= src->length) return false;

    char_t* ptr = src->data;
    const u64 delta = end + 1 - start;
    src->length -= delta;

    if (!src->length) return true;
    for (u64 i = start; i < end + 1; i++) ptr[i] = ptr[i + delta];
    return true;
}
str_t* get_substr(str_t* src, const u64 start, const u64 end) {
    if (!src || start >= src->length || end >= src->length) return false;
    return new_str(src->data + start * sizeof(char_t), end - start);
}
bool push_char(str_t* src, const char_t c) {
    if (!src) return false;
    if (src->capacity <= src->length && !__resize_string(src)) goto cleanup;

    src->data[src->length++] = c;
    return true;
cleanup:
    logError("push_char - Failed to resize string buffer.");
    return false;
}
bool insert_char(str_t* src, const u64 index, const char_t c) {
    if (!src || index > src->length) return false;
    if (src->capacity <= src->length && !__resize_string(src)) return false;

    char_t* ptr = src->data;
    memmove(&ptr[index + 1], &ptr[index], (src->length - index) * sizeof(char_t));

    src->data[index] = c;
    src->length++;
    return true;
cleanup:
    logError("insert_char - Failed to resize string buffer.");
    return false;
}
bool concat_str(str_t* dst, str_t* src) {
    if (!dst || !src) return false;

    const u64 index = dst->length;
    dst->length += src->length;
    if (dst->capacity <= dst->length && !__resize_string(dst)) goto cleanup;
    memcpy_s(dst->data + index, dst->length, src->data, src->length);

    return true;
cleanup:
    dst->length -= src->length;
    logError("concat_str - Failed to resize source string buffer.");
    return false;
}
u64 find_char(str_t* src, const u64 start, const char_t c) {
    if (!src) return 0;
    u64 i = start;
    for (; src->data[i] != c && i < src->length; i++) {}
    return i;
}
u64 rfind_char(str_t* src, const u64 start, const char_t c) {
    if (!src) return 0;
    u64 i = start;
    for (; src->data[i] != c && (i64)i > 0; i--);
    if (i) i++;
    return i;
}

void prints(const str_t* src, const bool new_line) {
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
