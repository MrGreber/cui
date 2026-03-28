#include <geometry/ops.h>
#include <log.h>
#include <memio.h>
#include <utils.h>
// todo: make this create no copies for of old vertex arrays and reuse the appropriate vertex arrays also cache it in the frame struct
vert_buf_t VertexBuffer(new)(const bool dynamic) {
    vert_buf_t vb = { 0 };
    u32 id;
    glcall(glGenBuffers(1, &id), cleanup, "VertexBuffer(new) - Failed to allocate vertex buffer.");
    vb.gl_id = id;
    vb.dynamic = dynamic;
    return vb;
cleanup:
    id = vb.gl_id;
    glDeleteBuffers(1, &id);
    vb.gl_id = 0;
    return (vert_buf_t){ 0 };
}
bool VertexBuffer(set)(vert_buf_t vb, const void* data, const u32 size) {
    if ((!data && !vb.dynamic) || !size) return false;
    glBindBuffer(GL_ARRAY_BUFFER, vb.gl_id);
    glcall(glBufferData(GL_ARRAY_BUFFER, size, data, vb.dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW), cleanup, "VertexBuffer(set) - Failed to copy vertex buffer data.");
    return true;
cleanup:
    return false;
}

elem_buf_t ElementBuffer(new)(void) {
    elem_buf_t eb = { 0 };
    glcall(glGenBuffers(1, &eb.id), cleanup, "ElementBuffer(new) - Failed to allocate element buffer.");
    return eb;
cleanup:
    glDeleteBuffers(1, &eb.id);
    return (elem_buf_t){ 0 };
}
bool ElementBuffer(set)(elem_buf_t eb, const u32* data, const u32 size) {
    if (!eb.id || !data || !size) return false;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eb.id);
    glcall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * sizeof(u32), data, GL_STATIC_DRAW), cleanup, "ElementBuffer(set) - Failed to copy element buffer data.");
    return true;
cleanup:
    return false;
}

vert_array_t* VertexArray(new)(u64 capacity) {
    if (!capacity) return NULL;

    buf_t buffer = {
        .size = sizeof(vert_array_t),
        .tag = MEMTAG_VERTEX_ARRAY
    };
    if (!new_buf(&buffer, false)) return NULL;

    capacity = __closest_pow2(capacity);
    vert_array_t* va = buffer.ptr;
    va->elem = 0;
    glcall(glGenVertexArrays(1, &va->id), cleanup, "VertexArray(new) - Failed to allocate vertex array.");

    buffer = (buf_t){
        .size = capacity * sizeof(vert_elem_t),
        .tag = MEMTAG_VERTEX_ARRAY_ELEMENT,
    };
    if (!new_buf(&buffer, false)) goto cleanup;

    va->stride = 0;
    va->count = 0;
    va->capacity = capacity;
    va->elem = buffer.ptr;

    return va;
cleanup:
    glDeleteVertexArrays(1, &va->id);
    if (va->elem) del_buf(&(buf_t){.size = capacity * sizeof(vert_elem_t), .tag = MEMTAG_VERTEX_ARRAY_ELEMENT, .ptr = va->elem});
    del_buf(&(buf_t){.size = sizeof(vert_array_t), .tag = MEMTAG_VERTEX_ARRAY, .ptr = va});
    return NULL;
}
void VertexArray(del)(vert_array_t* va) {
    if (!va) return;

    glDeleteVertexArrays(1, &va->id);
    glBindVertexArray(0);

    del_buf(&(buf_t){.size = va->capacity * sizeof(vert_elem_t), .tag = MEMTAG_VERTEX_ARRAY_ELEMENT, .ptr = va->elem});
    del_buf(&(buf_t){.size = sizeof(vert_array_t), .tag = MEMTAG_VERTEX_ARRAY, .ptr = va});
}

static bool private(resize_vertex_array)(vert_array_t* va) {
    if (va->count >= va->capacity) {
        const u32 new_capacity = va->capacity << 1;
        buf_t buffer = {
            .size = va->capacity * sizeof(vert_elem_t),
            .tag = MEMTAG_VERTEX_ARRAY_ELEMENT,
            .ptr = va->elem
        };
        if (!renew_buf(&buffer, new_capacity * sizeof(vert_elem_t))) return false;

        va->elem = buffer.ptr;
        va->capacity = new_capacity;
    }
    return true;
}
void VertexArray(push_f32)(vert_array_t* va, const u32 count) {
    if (!va || !count) return;
    if (!private(resize_vertex_array)(va)) return;
    vert_elem_t* elem = &va->elem[va->count];
    elem->count = count;
    elem->gl_type = GL_FLOAT;
    elem->normalized = false;
    va->count++;
    va->stride += sizeof(f32) * count;
}
void VertexArray(push_u32)(vert_array_t* va, const u32 count) {
    if (!va || !count) return;
    if (private(resize_vertex_array)(va)) return;

    vert_elem_t* elem = &va->elem[va->count];
    elem->count = count;
    elem->gl_type = GL_UNSIGNED_INT;
    elem->normalized = false;
    va->count++;
    va->stride += sizeof(u32) * count;
}
void VertexArray(push_u8)(vert_array_t* va, const u32 count) {
    if (!va || !count) return;
    if (private(resize_vertex_array)(va)) return;

    vert_elem_t* elem = &va->elem[va->count];
    elem->count = count;
    elem->gl_type = GL_UNSIGNED_BYTE;
    elem->normalized = false;
    va->count++;
    va->stride += sizeof(u8) * count;
}

static u32 private(gl_sizeof)(const u32 type) {
    switch (type) {
        case GL_FLOAT:
        case GL_UNSIGNED_INT: return 4;
        case GL_UNSIGNED_BYTE: return 1;
        default: return 0;
    }
}
void VertexArray(push_buffer)(vert_array_t* va, vert_buf_t vb) {
    if (!va || !vb.id) return;

    VertexArray(bind)(va);
    VertexBuffer(bind)(vb);

    const vert_elem_t* elements = va->elem;
    u32 offset = 0;
    for (u32 i = 0; i < va->count; i++) {
        const vert_elem_t* elem = &elements[i];
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(
            i,
            elem->count,
            elem->gl_type,
            elem->normalized,
            va->stride,
            (const void*)(u64)offset
        );
        offset += elem->count * private(gl_sizeof)(elem->type);
    }
}

mesh_t* Mesh(new)(frame_t* frame, const mesh_tag_t tag) {
    mesh_t* mesh = NULL;
    if (tag < __MESH_TAG_COUNT__ && frame->cache.static_meshes[tag].va == NULL) {
        static_mesh_t* static_mesh = &frame->cache.static_meshes[tag];
        static_mesh->va = VertexArray(new)(2);
        static_mesh->vb = VertexBuffer(new)(false);

    }
    else if (tag == DYNAMIC_MESH) {

    }

    return mesh;
cleanup:
    return NULL;
}
void Mesh(del_cache)(frame_t* frame) {

}