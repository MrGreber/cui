#include <memio.h>
#include <log.h>

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

static u64 mem_table[__MEMTAG_COUNT__ - 1] = { 0 };
static char* mem_table_labels[__MEMTAG_COUNT__ - 1] = {
    "vertex-buffer",
    "element-buffer",
    "vertex-array",
    "vertex-array-element",
    "canvas",
    "color",
    "shader",
    "sprite",
    "camera",
    "byte",
    "frame",
    "texture",
    "button",
    "component-node",
    "pointer",
    "panel",
    "app",
    "array",
    "edit",
    "string",
    "font",
    "vector",
    "hashmap",
    "key-value-pair",
    "mesh"
};


bool new_buf(buf_t* buffer, const bool zero) {
    if (!buffer) {
        logWarn("new_buf - Invalid address NULL.");
        return false;
    }
    if (!buffer->size || !buffer->tag) {
        logWarn("new_buf - Missing field initializations.");
        return false;
    }

    switch (buffer->tag) {
        case MEMTAG_COLOR: {
            buffer->ptr = _mm_malloc(buffer->size, 64);
            break;
        }
        default: {
            buffer->ptr = zero ? calloc(1, buffer->size) : malloc(buffer->size);
            break;
        }
    }
    if (!buffer->ptr) {
        logError("new_buf - Failed to allocate memory.");
        return false;
    }
    mem_table[buffer->tag - 1] += buffer->size;
    return true;
}
bool renew_buf(buf_t* buffer, const u64 new_size) {
    if (!buffer) {
        logWarn("renew_buf - Invalid address NULL.");
        return false;
    }
    if (!buffer->size || !buffer->tag || !buffer->ptr) {
        logWarn("renew_buf - Missing field initializations.");
        return false;
    }
    if (new_size <= buffer->size) {
        logWarn("renew_buf - No Change to memory size.");
        return false;
    }

    void* new_ptr = NULL;
    switch (buffer->tag) {
        case MEMTAG_COLOR: {
            new_ptr = _aligned_realloc(buffer->ptr, new_size, 64);
            break;
        }
        default: {
            new_ptr = realloc(buffer->ptr, new_size);
            break;
        }
    }
    if (!new_ptr) {
        logError("renew_buf - Failed to reallocate memory.");
        return false;
    }
    buffer->ptr = new_ptr;
    mem_table[buffer->tag - 1] += new_size - buffer->size;
    buffer->size = new_size;
    return true;
}
void del_buf(buf_t* buffer) {
    if (!buffer) return;

    switch (buffer->tag) {
        case MEMTAG_COLOR: {
            _mm_free(buffer->ptr);
            break;
        }
        default: {
            free(buffer->ptr);
            break;
        }
    }
    buffer->ptr = NULL;
    mem_table[buffer->tag - 1] -= buffer->size;
}

void print_memtable(void) {
    u64 max_len = 0;
    for (u64 i = 0; i < sizeof(mem_table_labels) / sizeof(char*); i++) {
        const u64 len = strlen(mem_table_labels[i]);
        if (len > max_len) max_len = len;
    }

    printf("%-*s | %10s\n", (int)max_len, "tag", "size");
    for (u64 i = 0; i < max_len + 13; i++) printf("-");
    printf("\n");

    for (u64 i = 0; i < sizeof(mem_table_labels) / sizeof(char*); i++) {
        printf("%-*s | %10llu\n", (int)max_len, mem_table_labels[i], mem_table[i]);
    }
}

bool read_file(const char* path, buf_t* buffer) {
    FILE* stream = NULL;

    if (fopen_s(&stream, path, "rb") != 0) {
        logError("read - Failed to open file: %s.", path);
        return false;
    }

    _fseeki64(stream, 0, SEEK_END);
    const i64 pos = _ftelli64(stream);
    if (pos == -1) goto cleanup;
    _fseeki64(stream, 0, SEEK_SET);

    buffer->size = pos;
    buffer->tag = MEMTAG_BYTE;
    if (!new_buf(buffer, true)) goto cleanup;
    if ((i64)fread(buffer->ptr, 1, pos, stream) != pos) goto cleanup;

    fclose(stream);
    return true;
cleanup:
    if (stream) fclose(stream);
    if (buffer->ptr) del_buf(buffer);
    return false;
}
