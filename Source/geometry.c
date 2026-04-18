#include <geometry/ops.h>
#include <error.h>
#include <memio.h>
#include <utils.h>
#include <frame.h>

#include <memory.h>

vert_buf_t VertexBuffer(new)(const bool dynamic, const void* vertices, const u32 size) {
    vert_buf_t vb = { 0 };
    u32 id;
    glcall(glGenBuffers(1, &id), cleanup, "Failed to allocate vertex buffer.");
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glcall(glBufferData(GL_ARRAY_BUFFER, size, vertices, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW), cleanup, "Failed to copy vertex buffer data.");
    vb.gl_id = id;
    vb.dynamic = dynamic;
    return vb;
cleanup:
    id = vb.gl_id;
    glDeleteBuffers(1, &id);
    vb.gl_id = 0;
    return (vert_buf_t){ 0 };
}

elem_buf_t ElementBuffer(new)(const bool dynamic, const u32* indices, const u32 size) {
    elem_buf_t eb = { 0 };
    u32 id;
    glcall(glGenBuffers(1, &id), cleanup, "Failed to allocate element buffer.");
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
    glcall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW), cleanup, "Failed to copy element buffer data.");
    eb.gl_id = id;
    eb.dynamic = dynamic;
    return eb;
cleanup:
    id = eb.gl_id;
    glDeleteBuffers(1, &id);
    eb.gl_id = 0;
    return (elem_buf_t){ 0 };
}

vert_array_t* VertexArray(new)(u64 capacity) {
    if (!capacity) return NULL;

    buf_t buffer = {
        .size = sizeof(vert_array_t),
        .tag = MEMTAG_VERTEX_ARRAY
    };
    if (!Buffer(new)(&buffer, false)) return NULL;

    capacity = __closest_pow2(capacity);
    vert_array_t* va = buffer.ptr;
    va->elem = 0;
    glcall(glGenVertexArrays(1, &va->id), cleanup, "Failed to allocate vertex array.");

    buffer = (buf_t){
        .size = capacity * sizeof(vert_elem_t),
        .tag = MEMTAG_VERTEX_ARRAY_ELEMENT,
    };
    if (!Buffer(new)(&buffer, false)) goto cleanup;

    va->stride = 0;
    va->count = 0;
    va->capacity = capacity;
    va->elem = buffer.ptr;

    return va;
cleanup:
    glDeleteVertexArrays(1, &va->id);
    if (va->elem) Buffer(del)(&(buf_t){.size = capacity * sizeof(vert_elem_t), .tag = MEMTAG_VERTEX_ARRAY_ELEMENT, .ptr = va->elem});
    Buffer(del)(&(buf_t){.size = sizeof(vert_array_t), .tag = MEMTAG_VERTEX_ARRAY, .ptr = va});
    return NULL;
}
void VertexArray(del)(vert_array_t* va) {
    if (!va) return;

    glDeleteVertexArrays(1, &va->id);
    glBindVertexArray(0);

    Buffer(del)(&(buf_t){.size = va->capacity * sizeof(vert_elem_t), .tag = MEMTAG_VERTEX_ARRAY_ELEMENT, .ptr = va->elem});
    Buffer(del)(&(buf_t){.size = sizeof(vert_array_t), .tag = MEMTAG_VERTEX_ARRAY, .ptr = va});
}

static bool private(resize_vertex_array)(vert_array_t* va) {
    if (va->count >= va->capacity) {
        const u32 new_capacity = va->capacity << 1;
        buf_t buffer = {
            .size = va->capacity * sizeof(vert_elem_t),
            .tag = MEMTAG_VERTEX_ARRAY_ELEMENT,
            .ptr = va->elem
        };
        if (!Buffer(renew)(&buffer, new_capacity * sizeof(vert_elem_t))) return false;

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

__forceinline u32 private(gl_sizeof)(const u32 type) {
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

struct mesh_range {
    struct { u32 offset, count; } vert;
    struct { u32 offset, count; } idx;
    u8 attributes;
};
const static struct mesh_range __mesh_table[__MESH_TAG_COUNT__] = {
    {0, 16, 0, 6, MESH_2D | MESH_UV0 | MESH_EB}
};
#include <geometry/static_meshes.h>

static const struct {
    u8 bit;
    u8 count;
} __attributes_table[] = {
    { MESH_2D, 2 },
    { MESH_3D, 3 },
    { MESH_UV0, 2 },
    { MESH_UV1, 2 },
    { MESH_TEX_IDX, 1 },
    { MESH_COLOR, 4 },
    { MESH_NORM, 3 }
};
static static_mesh_t* private(new_static_mesh)(frame_t* frame, const mesh_tag_t tag) {
    static_mesh_t* static_mesh = &frame->cache.static_meshes.data[tag];
    if (static_mesh->va) return static_mesh;
    static_mesh->va = VertexArray(new)(2);
    if (!static_mesh->va->id) {
        logError(ERR_OPENGL, "Failed to create vertex array for static mesh.");
        goto static_cleanup;
    }

    const struct mesh_range* range = &__mesh_table[tag];
    const f32* vertices = (f32*)__vertices + range->vert.offset;
    const u32 count = range->vert.count;
    static_mesh->vb = VertexBuffer(new)(false, vertices, count * sizeof(f32));
    if (!static_mesh->vb.id) {
        logError(ERR_OPENGL, "Failed to create vertex buffer for static mesh.");
        goto static_cleanup;
    }
    VertexArray(bind)(static_mesh->va);
    VertexBuffer(bind)(static_mesh->vb);

    for (u8 i = 0; i < 7; i++) {
        if (range->attributes & __attributes_table[i].bit) {
            VertexArray(push_f32)(static_mesh->va, __attributes_table[i].count);
        }
    }
    VertexArray(push_buffer)(static_mesh->va, static_mesh->vb);

    if (!frame->cache.static_meshes.eb.id) {
        frame->cache.static_meshes.eb = ElementBuffer(new)(false, __indices, sizeof(__indices));
        if (!frame->cache.static_meshes.eb.id) {
            logError(ERR_OPENGL, "Failed to create element buffer for static mesh.");
            goto static_cleanup;
        }
    }
    static_mesh->tag = tag;
    return static_mesh;
static_cleanup:
    Mesh(del_cache)(frame);
    return NULL;
}
static dynamic_mesh_t* private(new_dynamic_mesh)(frame_t* frame, const mesh_param_t params) {
    if (frame->cache.dynamic_meshes.count >= frame->cache.dynamic_meshes.capacity) {
        const u32 new_capacity = frame->cache.dynamic_meshes.capacity << 1;
        buf_t buffer = {
            .ptr = frame->cache.dynamic_meshes.data,
            .size = frame->cache.dynamic_meshes.capacity * sizeof(dynamic_mesh_t),
            .tag = MEMTAG_MESH
        };
        if (!Buffer(renew)(&buffer, new_capacity * sizeof(dynamic_mesh_t))) {
            Mesh(del_cache)(frame);
            return NULL;
        }
        frame->cache.dynamic_meshes.data = buffer.ptr;
        frame->cache.dynamic_meshes.capacity = new_capacity;
    }
    dynamic_mesh_t* dynamic_mesh = &frame->cache.dynamic_meshes.data[frame->cache.dynamic_meshes.count++];
    mesh_metadata_t* metadata = &dynamic_mesh->metadata;
    metadata->va = VertexArray(new)(2);
    if (!metadata->va->id) {
        logError(ERR_OPENGL, "Failed to create vertex array for dynamic mesh.");
        goto dynamic_cleanup;
    }

    buf_t buffer = {
        .size = params.capacity * sizeof(f32),
        .tag = MEMTAG_MESH,
    };
    if (!Buffer(new)(&buffer, false)) {
        logError(ERR_HEAP_ALLOC, "Failed to allocate dynamic mesh vertices array.");
        goto dynamic_cleanup;
    }
    dynamic_mesh->vertices.data = buffer.ptr;

    metadata->vb = VertexBuffer(new)(true, NULL, params.capacity * sizeof(f32));
    if (!metadata->vb.id) {
        logError(ERR_OPENGL, "Failed to create vertex buffer for dynamic mesh.");
        goto dynamic_cleanup;
    }
    VertexArray(bind)(metadata->va);
    VertexBuffer(bind)(metadata->vb);



    for (u8 i = 0; i < 7; i++) {
        if (params.attributes & __attributes_table[i].bit) {
            VertexArray(push_f32)(metadata->va, __attributes_table[i].count);
        }
    }
    VertexArray(push_buffer)(metadata->va, metadata->vb);

    if (params.attributes & MESH_EB) {
        dynamic_mesh->eb = ElementBuffer(new)(true, NULL, params.capacity * sizeof(u32));
        if (!dynamic_mesh->eb.id) {
            logError(ERR_OPENGL, "Failed to create element buffer for dynamic mesh.");
            goto dynamic_cleanup;
        }
        buffer = (buf_t){
            .size = params.capacity * sizeof(u32),
            .tag = MEMTAG_MESH,
        };
        if (!Buffer(new)(&buffer, false)) {
            logError(ERR_HEAP_ALLOC, "Failed to allocate dynamic mesh indices array.");
            goto dynamic_cleanup;
        }
        dynamic_mesh->indices.capacity = params.capacity;
        dynamic_mesh->indices.data = buffer.ptr;
    }
    metadata->tag = DYNAMIC_MESH;
    dynamic_mesh->vertices.capacity = params.capacity;
    return dynamic_mesh;
dynamic_cleanup:
    Mesh(del_cache)(frame);
    return NULL;
}
mesh_t* Mesh(new)(frame_t* frame, const mesh_param_t params) {
    if (params.tag < __MESH_TAG_COUNT__) return (mesh_t*)private(new_static_mesh)(frame, params.tag);
    else if (params.tag == DYNAMIC_MESH) return private(new_dynamic_mesh)(frame, params);
    return NULL;
}
bool Mesh(new_cache)(frame_t* frame) {
#define DEFAULT_CAPACITY 4
    buf_t buffer = { .size = sizeof(dynamic_mesh_t) * DEFAULT_CAPACITY, .tag = MEMTAG_MESH };
    if (!Buffer(new)(&buffer, false)) {
        logError(ERR_HEAP_ALLOC, "Failed to allocate dynamic meshes cache.");
        return false;
    }
    frame->cache.dynamic_meshes.data = buffer.ptr;
    frame->cache.dynamic_meshes.capacity = DEFAULT_CAPACITY;
    frame->cache.dynamic_meshes.count = 0;
    return true;

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
        if (dynamic_mesh->eb.gl_id) ElementBuffer(del)(&dynamic_mesh->eb);
        if (dynamic_mesh->indices.data) {
            Buffer(del)(&(buf_t){
                .ptr = dynamic_mesh->indices.data,
                .size = dynamic_mesh->indices.capacity * sizeof(vec4),
                .tag = MEMTAG_VECTOR
            });
        }
        if (dynamic_mesh->vertices.data) {
            Buffer(del)(&(buf_t){
                .ptr = dynamic_mesh->vertices.data,
                .size = dynamic_mesh->vertices.capacity * sizeof(vec4),
                .tag = MEMTAG_VECTOR
            });
        }
    }
    if (frame->cache.dynamic_meshes.data) {
        Buffer(del)(&(buf_t){
            .ptr = frame->cache.dynamic_meshes.data,
            .size = frame->cache.dynamic_meshes.capacity * sizeof(dynamic_mesh_t),
            .tag = MEMTAG_MESH
        });
    }
}
void Mesh(bind)(const frame_t* frame, const mesh_t* mesh) {
    const mesh_metadata_t* metadata = (mesh_metadata_t*)mesh;
    VertexArray(bind)(metadata->va);
    if (metadata->tag < __MESH_TAG_COUNT__) ElementBuffer(bind)(frame->cache.static_meshes.eb);
    else if (mesh->eb.id) ElementBuffer(bind)(mesh->eb);
}

bool Mesh(write)(mesh_t* mesh, const mesh_buf_t* mesh_buffer) {
    const u32 stride = mesh_buffer->count - mesh_buffer->offset;

    if (mesh_buffer->is_vertices) {
        if (mesh->vertices.count + stride >= mesh->vertices.capacity) {
            u32 new_capacity = mesh->vertices.capacity << 1;
            while (new_capacity > mesh->vertices.count + stride) new_capacity <<= 1;

            buf_t buffer = {
                .size = mesh->vertices.capacity * sizeof(f32),
                .tag = MEMTAG_MESH,
                .ptr = mesh->vertices.data
            };
            if (!Buffer(renew)(&buffer, new_capacity * sizeof(f32))) {
                logError(ERR_HEAP_REALLOC, "Failed to reallocate dynamic mesh vertices array.");
                return false;
            }

            mesh->vertices.data = buffer.ptr;
            mesh->vertices.capacity = new_capacity;
        }

        memcpy(mesh->vertices.data + mesh_buffer->offset, mesh_buffer->data, mesh_buffer->count * sizeof(f32));
        mesh->vertices.count += stride;
    }
    else {
        if (mesh->indices.count + stride >= mesh->indices.capacity) {
            u32 new_capacity = mesh->indices.capacity << 1;
            while (new_capacity > mesh->indices.count + stride) new_capacity <<= 1;
            buf_t buffer = {
                .size = mesh->indices.capacity * sizeof(u32),
                .tag = MEMTAG_MESH,
                .ptr = mesh->indices.data
            };
            if (!Buffer(renew)(&buffer, new_capacity * sizeof(u32))) {
                logError(ERR_HEAP_REALLOC, "Failed to reallocate dynamic mesh indices array.");
                return false;
            }

            mesh->indices.data = buffer.ptr;
            mesh->indices.capacity = new_capacity;
        }

        memcpy(mesh->vertices.data + mesh_buffer->offset, mesh_buffer->data, mesh_buffer->count * sizeof(u32));
        mesh->vertices.count += stride;
    }
    return true;
}
bool Mesh(push)(mesh_t* mesh, const mesh_buf_t* mesh_buffer) {
    if (mesh_buffer->is_vertices) {
        if (mesh->vertices.count + mesh_buffer->count >= mesh->vertices.capacity) {
            u32 new_capacity = mesh->vertices.capacity << 1;
            while (new_capacity > mesh->vertices.count + mesh_buffer->count) new_capacity <<= 1;

            buf_t buffer = {
                .size = mesh->vertices.capacity * sizeof(f32),
                .tag = MEMTAG_MESH,
                .ptr = mesh->vertices.data
            };
            if (!Buffer(renew)(&buffer, new_capacity * sizeof(f32))) {
                logError(ERR_HEAP_REALLOC, "Failed to reallocate dynamic mesh vertices array.");
                return false;
            }

            mesh->vertices.data = buffer.ptr;
            mesh->vertices.capacity = new_capacity;
        }

        memcpy(mesh->vertices.data + mesh->vertices.count, mesh_buffer->data, mesh_buffer->count * sizeof(f32));
        mesh->vertices.count += mesh_buffer->count;
    }
    else {
        if (mesh->indices.count + mesh_buffer->count >= mesh->indices.capacity) {
            u32 new_capacity = mesh->indices.capacity << 1;
            while (new_capacity > mesh->indices.count + mesh_buffer->count) new_capacity <<= 1;
            buf_t buffer = {
                .size = mesh->indices.capacity * sizeof(u32),
                .tag = MEMTAG_MESH,
                .ptr = mesh->indices.data
            };
            if (!Buffer(renew)(&buffer, new_capacity * sizeof(u32))) {
                logError(ERR_HEAP_REALLOC, "Failed to reallocate dynamic mesh indices array.");
                return false;
            }

            mesh->indices.data = buffer.ptr;
            mesh->indices.capacity = new_capacity;
        }

        memcpy(mesh->vertices.data + mesh->indices.count, mesh_buffer->data, mesh_buffer->count * sizeof(u32));
        mesh->vertices.count += mesh_buffer->count;
    }
    return true;
}
void Mesh(draw)(mesh_t* mesh) {
    if (!mesh) return;
    mesh_metadata_t* metadata = &mesh->metadata;
    if (metadata->tag < __MESH_TAG_COUNT__) {
        const struct mesh_range* range = &__mesh_table[metadata->tag];
        if (range->idx.count) {
            glDrawElements(GL_TRIANGLES, range->idx.count, GL_UNSIGNED_INT, (void*)(range->idx.offset * sizeof(u32)));
        }
        else glDrawArrays(GL_TRIANGLES, 0, range->vert.count);
    }
    else {
        if (mesh->eb.gl_id) glDrawElements(GL_TRIANGLES, mesh->indices.count, GL_UNSIGNED_INT, 0);
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
        if (mesh->eb.gl_id) glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, (void*)(offset * sizeof(u32)));
        else glDrawArrays(GL_TRIANGLES, offset, count);
    }
}