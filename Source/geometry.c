#include <geometry/ops.h>
#include <log.h>
#include <memio.h>
#include <utils.h>
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
bool VertexBuffer(init)(vert_buf_t vb, const void* vertices, const u32 size) {
    if ((!vertices && !vb.dynamic) || !size) return false;
    glBindBuffer(GL_ARRAY_BUFFER, vb.gl_id);
    glcall(glBufferData(GL_ARRAY_BUFFER, size, vertices, vb.dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW), cleanup, "VertexBuffer(set) - Failed to copy vertex buffer data.");
    return true;
cleanup:
    return false;
}

elem_buf_t ElementBuffer(new)(const bool dynamic) {
    elem_buf_t eb = { 0 };
    u32 id;
    glcall(glGenBuffers(1, &id), cleanup, "ElementBuffer(new) - Failed to allocate element buffer.");
    eb.gl_id = id;
    eb.dynamic = dynamic;
    return eb;
cleanup:
    glDeleteBuffers(1, &eb.id);
    return (elem_buf_t){ 0 };
}
bool ElementBuffer(init)(elem_buf_t eb, const u32* indices, const u32 size) {
    if (!eb.id || !indices || !size) return false;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eb.id);
    glcall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, eb.dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW), cleanup, "ElementBuffer(set) - Failed to copy element buffer data.");
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

#define MESH_2D 1
#define MESH_3D 2
#define MESH_UV 4
#define MESH_NORM 8
struct mesh_range {
    struct { u32 offset, count; } vert;
    struct { u32 offset, count; } idx;
    u8 attributes;
};
const static u32 __indices[] = {
    // rectangle
    2,1,0,
    2,3,1
};
const static f32 __vertices[] = {
    // rectangle
    0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f
};
const static struct mesh_range __mesh_table[__MESH_TAG_COUNT__] = {
    {0, 16, 0, 6, MESH_2D | MESH_UV}
};

static const struct { u8 bit; u8 count; } __attributes_table[] = {
    { MESH_2D,   2 },
    { MESH_3D,   3 },
    { MESH_UV,   2 },
    { MESH_NORM, 3 },
};
static static_mesh_t* private(new_static_mesh)(frame_t* frame, const mesh_tag_t tag) {
    static_mesh_t* static_mesh = &frame->cache.static_meshes.data[tag];
    static_mesh->va = VertexArray(new)(2);
    if (!static_mesh->va->id) {
        logError("Mesh(new) - Failed to create vertex array for static mesh.");
        goto static_cleanup;
    }
    static_mesh->vb = VertexBuffer(new)(false);
    if (!static_mesh->vb.id) {
        logError("Mesh(new) - Failed to create vertex buffer for static mesh.");
        goto static_cleanup;
    }
    const struct mesh_range* range = &__mesh_table[tag];
    VertexArray(bind)(static_mesh->va);
    const f32* vertices = (f32*)__vertices + range->vert.offset;
    const u32 count = range->vert.count;
    VertexBuffer(bind)(static_mesh->vb);
    VertexBuffer(init)(static_mesh->vb, vertices, count * sizeof(f32));

    for (u8 i = 0; i < 4; i++) {
        if (range->attributes & __attributes_table[i].bit)
            VertexArray(push_f32)(static_mesh->va, __attributes_table[i].count);
    }
    VertexArray(push_buffer)(static_mesh->va, static_mesh->vb);

    if (!frame->cache.static_meshes.eb.id) {
        frame->cache.static_meshes.eb = ElementBuffer(new)(false);
        if (!frame->cache.static_meshes.eb.id) {
            logError("Mesh(new) - Failed to create element buffer for static mesh.");
            goto static_cleanup;
        }
        ElementBuffer(init)(frame->cache.static_meshes.eb, __indices, sizeof(__indices));
    }
    static_mesh->tag = tag;
    return static_mesh;
static_cleanup:
    Mesh(del_cache)(frame);
    return NULL;
}
static dynamic_mesh_t* private(new_dynamic_mesh)(frame_t* frame) {
    return NULL;
}
mesh_t* Mesh(new)(frame_t* frame, const mesh_tag_t tag) {
    if (tag < __MESH_TAG_COUNT__ && frame->cache.static_meshes.data[tag].va == NULL) return (mesh_t*)private(new_static_mesh)(frame, tag);
    else if (tag == DYNAMIC_MESH) return private(new_dynamic_mesh)(frame);
    return NULL;
}
void Mesh(del_cache)(frame_t* frame) {
    for (mesh_tag_t i = 0; i < __MESH_TAG_COUNT__; i++) {
        static_mesh_t* static_mesh = &frame->cache.static_meshes.data[i];
        if (static_mesh->vb.id) {
            VertexArray(del)(static_mesh->va);
            VertexBuffer(del)(&static_mesh->vb);
        }
    }
    if (frame->cache.static_meshes.eb.id) ElementBuffer(del)(&frame->cache.static_meshes.eb);
    for (u16 i = 0; i < frame->cache.dynamic_meshes.count; i++) {
        dynamic_mesh_t* dynamic_mesh = &frame->cache.dynamic_meshes.data[i];
        mesh_metadata_t* metadata = &dynamic_mesh->metadata;

        if (metadata->vb.id) {
            VertexArray(del)(metadata->va);
            VertexBuffer(del)(&metadata->vb);
        }
        if (dynamic_mesh->eb.id) ElementBuffer(del)(&dynamic_mesh->eb);
        if (dynamic_mesh->indices.data) {
            del_buf(&(buf_t){
                .ptr = dynamic_mesh->indices.data,
                .size = dynamic_mesh->indices.capacity * sizeof(vec4),
                .tag = MEMTAG_VECTOR
            });
        }
        if (dynamic_mesh->vertices.data) {
            del_buf(&(buf_t){
                .ptr = dynamic_mesh->vertices.data,
                .size = dynamic_mesh->vertices.capacity * sizeof(vec4),
                .tag = MEMTAG_VECTOR
            });
        }
    }
    if (frame->cache.dynamic_meshes.data) {
        del_buf(&(buf_t){
            .ptr = frame->cache.dynamic_meshes.data,
            .size = frame->cache.dynamic_meshes.capacity * sizeof(dynamic_mesh_t),
            .tag = MEMTAG_MESH
        });
    }
}

void Mesh(draw)(mesh_t* mesh) {
    if (!mesh) return;
    mesh_metadata_t* metadata = &mesh->metadata;
    if (metadata->tag < __MESH_TAG_COUNT__) {
        const struct mesh_range* range = &__mesh_table[metadata->tag];
        if (range->idx.count) glDrawElements(GL_TRIANGLES, range->vert.count, GL_UNSIGNED_INT, 0);
        else glDrawArrays(GL_TRIANGLES, 0, range->vert.count);
    }
    else {
        if (mesh->eb.id) glDrawElements(GL_TRIANGLES, mesh->indices.count, GL_UNSIGNED_INT, 0);
        else glDrawArrays(GL_TRIANGLES, 0, mesh->vertices.count);
    }
}
void Mesh(sub_draw)(mesh_t* mesh, const u32 count, const u32 offset) {
    if (!mesh) return;
    mesh_metadata_t* metadata = &mesh->metadata;
    if (metadata->tag < __MESH_TAG_COUNT__) {
        const struct mesh_range* range = &__mesh_table[metadata->tag];
        if (range->idx.count && offset + count <= range->idx.offset + range->idx.count) glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, (void*)(offset * sizeof(u32)));
        else if (offset + count <= range->vert.offset + range->vert.count) glDrawArrays(GL_TRIANGLES, offset, count);
    }
    else {
        if (mesh->eb.id) glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, (void*)(offset * sizeof(u32)));
        else glDrawArrays(GL_TRIANGLES, offset, count);
    }
}