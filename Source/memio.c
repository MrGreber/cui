#include <memio.h>
#include <error.h>

#include <stdio.h>
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
    "caption",
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
    "mesh",
    "vertex"
};


bool Buffer(new)(buf_t* buffer, const bool zero) {
    if (!buffer) {
        logWarn(ERR_INVALID_PARAM, "Address 0x%p buffer.", NULL);
        return false;
    }
    if (!buffer->size || !buffer->tag) {
        logWarn(ERR_INVALID_PARAM, "Missing field initializations.");
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
        logError(ERR_HEAP_ALLOC, "Failed to allocate %d\n", buffer->size);
        return false;
    }
    mem_table[buffer->tag - 1] += buffer->size;
    return true;
}
bool Buffer(renew)(buf_t* buffer, const u64 new_size) {
    if (!buffer) {
        logWarn(ERR_INVALID_PARAM, "Address 0x%p buffer.", NULL);
        return false;
    }
    if (!buffer->size || !buffer->tag || !buffer->ptr) {
        logWarn(ERR_INVALID_PARAM, "Missing field initializations.");
        return false;
    }
    if (new_size <= buffer->size) {
        logWarn(ERR_HEAP_REALLOC, "No Change to memory size.");
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
        logError(ERR_HEAP_REALLOC, "");
        return false;
    }
    buffer->ptr = new_ptr;
    mem_table[buffer->tag - 1] += new_size - buffer->size;
    buffer->size = new_size;
    return true;
}
void Buffer(del)(buf_t* buffer) {
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

u64 get_memory_usage(const bool detailed) {
    static u16 max_len = 0;

    if (!max_len) {
        for (u16 i = 0; i < sizeof(mem_table_labels) / sizeof(char*); i++) {
            const u16 len = strlen(mem_table_labels[i]);
            if (len > max_len) max_len = len;
        }
    }

    u64 memory_usage = 0;
    if (detailed) {
        printf("%-*s | %11s\n", (int)max_len, "tag", "size");

        byte sep[max_len + 18];
        memset(sep, '-', max_len + 18);
        sep[max_len + 17] = '\n';
        fwrite(sep, 1, max_len + 18, stdout);

        for (u64 i = 0; i < sizeof(mem_table_labels) / sizeof(char*); i++) {
            u64 size = mem_table[i];
            memory_usage += size;

            char* unit;
            if (size < (1ull << 10)) {
                unit = " B";
            }
            else if (size < (1ull << 20)) {
                unit = " KiB";
                size >>= 10;
            }
            else {
                unit = " MiB";
                size >>= 20;
            }

            printf("%-*s | %10llu%s\n", (int)max_len, mem_table_labels[i], size, unit);
        }
    }
    else {
        for (u64 i = 0; i < sizeof(mem_table_labels) / sizeof(char*); i++) {
            u64 size = mem_table[i];
            memory_usage += size;
            // if (size < (1ull << 10)) size = size;
            // if (size < (1ull << 20)) size >>= 10;
            // else size >>= 20;
        }
    }
    return memory_usage;
}

bool read_file(const char* path, buf_t* buffer) {
    FILE* stream = NULL;

    if (fopen_s(&stream, path, "rb") != 0) {
        logError(ERR_FILE_OPEN, "Failed to read file: %s.", path);
        return false;
    }

    _fseeki64(stream, 0, SEEK_END);
    const i64 pos = _ftelli64(stream);
    if (pos == -1) goto cleanup;
    _fseeki64(stream, 0, SEEK_SET);

    buffer->size = pos;
    buffer->tag = MEMTAG_BYTE;
    if (!Buffer(new)(buffer, true)) goto cleanup;
    if ((i64)fread(buffer->ptr, 1, pos, stream) != pos) goto cleanup;

    fclose(stream);
    return true;
cleanup:
    if (stream) fclose(stream);
    if (buffer->ptr) Buffer(del)(buffer);
    return false;
}
