#include <geometry.h>
#include <memio.h>
#include <log.h>
#include <utils.h>

#include <glad.h>


static u32 __gl_sizeof(const u32 type) {
    switch (type) {
        case GL_FLOAT:
        case GL_UNSIGNED_INT: return 4;
        case GL_UNSIGNED_BYTE: return 1;
        default: return 0;
    }
}

vert_buf* new_vertex_buffer(const void* data, const u32 size, const u8 type) {
    if (!size) return NULL;
    if (!data && type == STATIC_BUFFER) {
        logWarn("new_vertex_buffer - Invalid data address NULL.");
        return NULL;
    }

    buf_t buffer = {
        .size = sizeof(vert_buf),
        .tag = MEMTAG_VERTEX_BUFFER
    };
    if (!new_buf(&buffer, false)) return NULL;

    vert_buf* vb = buffer.ptr;
    vb->type = type;
    glcall(glGenBuffers(1, &vb->id), cleanup, "new_vertex_buffer - Failed to allocate vertex buffer.");
    glBindBuffer(GL_ARRAY_BUFFER, vb->id);
    glcall(glBufferData(GL_ARRAY_BUFFER, size, data, type ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW), cleanup, "new_vertex_buffer - Failed to copy vertex buffer data.");

    return vb;
cleanup:
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vb->id);
    del_buf(&buffer);
    return NULL;
}
void del_vertex_buffer(vert_buf* vb) {
    if (!vb) return;

    glDeleteBuffers(1, &vb->id);

    buf_t buffer = {
        .size = sizeof(vert_buf),
        .tag = MEMTAG_VERTEX_BUFFER,
        .ptr = vb
    };
    del_buf(&buffer);
}
void bind_vertex_buffer(const vert_buf* vb) {
    glBindBuffer(GL_ARRAY_BUFFER, vb->id);
}
void unbind_vertex_buffer() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

elem_buf* new_element_buffer(const u32* data, const u32 count) {
    if (!count) return NULL;
    if (data == NULL) {
        logWarn("new_vertex_buffer - Invalid data address NULL.");
        return NULL;
    }

    buf_t buffer = {
        .size = sizeof(elem_buf),
        .tag = MEMTAG_ELEMENT_BUFFER
    };
    if (!new_buf(&buffer, false)) return NULL;

    elem_buf* eb = buffer.ptr;
    glcall(glGenBuffers(1, &eb->id), cleanup, "new_element_buffer - Failed to allocate element buffer.");
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eb->id);
    glcall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(u32), data, GL_STATIC_DRAW), cleanup, "new_element_buffer - Failed to copy element buffer data.");

    eb->count = count;
    return eb;
cleanup:
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &eb->id);
    del_buf(&buffer);
    return NULL;
}
void del_element_buffer(elem_buf* eb) {
    if (!eb) return;

    glDeleteBuffers(1, &eb->id);
    buf_t buffer = {
        .size = sizeof(elem_buf),
        .tag = MEMTAG_ELEMENT_BUFFER,
        .ptr = eb
    };

    del_buf(&buffer);
}
void bind_element_buffer(const elem_buf* vb) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vb->id);
}
void unbind_element_buffer() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

vert_array* new_vertex_array(const u64 capacity) {
    if (!capacity) return NULL;

    buf_t buffer = {
        .size = sizeof(vert_array),
        .tag = MEMTAG_VERTEX_ARRAY
    };
    if (!new_buf(&buffer, false)) return NULL;

    const u64 cap = __closest_pow2(capacity);
    vert_array* va = buffer.ptr;
    va->elem = 0;
    glcall(glGenVertexArrays(1, &va->id), cleanup, "new_vertex_array - Failed to allocate vertex array.");

    buffer = (buf_t){
        .size = cap * sizeof(vert_elem),
        .tag = MEMTAG_VERTEX_ARRAY_ELEMENT,
    };
    if (!new_buf(&buffer, false)) goto cleanup;

    va->stride = 0;
    va->count = 0;
    va->capacity = cap;
    va->elem = buffer.ptr;

    return va;
cleanup:
    glDeleteVertexArrays(1, &va->id);
    if (va->elem) del_buf(&(buf_t){.size = sizeof(vert_elem), .tag = MEMTAG_VERTEX_ARRAY_ELEMENT, .ptr = va->elem});
    del_buf(&(buf_t){.size = sizeof(vert_array), .tag = MEMTAG_VERTEX_ARRAY, .ptr = va});
    return NULL;
}
void del_vertex_array(vert_array* va) {
    if (!va) return;

    glDeleteVertexArrays(1, &va->id);
    glBindVertexArray(0);

    del_buf(&(buf_t){
        .size = va->capacity * sizeof(vert_elem),
        .tag = MEMTAG_VERTEX_ARRAY_ELEMENT,
        .ptr = va->elem
    });

    del_buf(&(buf_t){
        .size = sizeof(vert_array),
        .tag = MEMTAG_VERTEX_ARRAY,
        .ptr = va
    });
}
void bind_vertex_array(const vert_array* va) {
    glBindVertexArray(va->id);
}
void unbind_vertex_array() {
    glBindVertexArray(0);
}

static bool __realloc_vertex_elements(vert_array* va) {
    if (va->count == va->capacity) {
        const u32 new_cap = va->capacity << 1;

        buf_t buffer = {
            .size = va->capacity * sizeof(vert_elem),
            .tag = MEMTAG_VERTEX_ARRAY_ELEMENT,
            .ptr = va->elem
        };
        if (!renew_buf(&buffer, new_cap * sizeof(vert_elem))) return false;

        va->elem = buffer.ptr;
        va->capacity = new_cap;
    }
    return true;
}
void push_f32(vert_array* va, const u32 count) {
    if (!va || !count) return;
    if (!__realloc_vertex_elements(va)) return;
    vert_elem* elem = &va->elem[va->count];
    elem->count = count;
    elem->type = GL_FLOAT;
    elem->normalized = false;
    va->count++;
    va->stride += sizeof(f32) * count;
}
void push_u32(vert_array* va, const u32 count) {
    if (!va || !count) return;
    if (__realloc_vertex_elements(va)) return;

    vert_elem* elem = &va->elem[va->count];
    elem->count = count;
    elem->type = GL_UNSIGNED_INT;
    elem->normalized = false;
    va->count++;
    va->stride += sizeof(u32) * count;
}
void push_u8(vert_array* va, const u32 count) {
    if (!va || !count) return;
    if (__realloc_vertex_elements(va)) return;

    vert_elem* elem = &va->elem[va->count];
    elem->count = count;
    elem->type = GL_UNSIGNED_BYTE;
    elem->normalized = false;
    va->count++;
    va->stride += sizeof(u8) * count;
}

void push_buf(vert_array* va, vert_buf* vb) {
    if (!va || !vb) return;

    bind_vertex_array(va);
    bind_vertex_buffer(vb);

    const vert_elem* elements = va->elem;
    u32 offset = 0;
    for (u32 i = 0; i < va->count; i++) {
        const vert_elem* elem = &elements[i];
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(
            i,
            elem->count,
            elem->type,
            elem->normalized,
            va->stride,
            (const void*)(u64)offset
        );
        offset += elem->count * __gl_sizeof(elem->type);
    }
}
