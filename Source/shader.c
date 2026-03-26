#include <shader/ops.h>
#include <log.h>
#include <memio.h>

#include <glad.h>
#include <string.h>

static u32 private(compile_shader)(const u32 type, const char* path) {
    buf_t buffer = { 0 };
    if (!read_file(path, &buffer)) {
        logFatal("private(compile_shader) - Failed to read shader glsl file.");
        return false;
    }
    const u32 id = glCreateShader(type);
    if (!id) goto cleanup;
    glcall(glShaderSource(id, 1, (const GLchar**)&buffer.ptr, NULL), cleanup, "private(compile_shader) - Failed to build shader: %s.", path);
    glcall(glCompileShader(id), cleanup, "private(compile_shader) - Failed to compile shader: %s.", path);

    i32 success = 0;
    glcall(glGetShaderiv(id, GL_COMPILE_STATUS, &success), cleanup, "private(compile_shader) - Failed to get shader: %s iv.", path);
    if (!success) {
        char msg[512] = { 0 };
        glGetShaderInfoLog(id, 512, NULL, msg);
        logError("private(compile_shader) - shader compilation error:\n%s", msg);
        goto cleanup;
    }
    del_buf(&buffer);
    return id;
cleanup:
    if (buffer.ptr) del_buf(&buffer);
    return 0;
}
static u32 private(link_shaders)(const char* vertex_path, const char* fragment_path) {
    const u32 vert_id = private(compile_shader)(GL_VERTEX_SHADER, vertex_path);
    const u32 frag_id = private(compile_shader)(GL_FRAGMENT_SHADER, fragment_path);
    if (vert_id == 0 || frag_id == 0) {
        logFatal("private(link_shader) - Failed to compile vertex/fragment shaders.");
        goto cleanup;
    }

    const u32 id = glCreateProgram();
    if (!id) goto cleanup;
    glcall(glAttachShader(id, vert_id), cleanup, "private(link_shader) - Failed to attach vertex shader.");
    glcall(glAttachShader(id, frag_id), cleanup, "private(link_shader) - Failed to attach fragment shader.");
    glcall(glLinkProgram(id), cleanup, "private(link_shader) - Failed to link shader.");

    i32 success = 0;
    glcall(glGetProgramiv(id, GL_LINK_STATUS, &success), cleanup, "private(link_shader) - Failed to get shader iv.");

    glDeleteShader(vert_id);
    glDeleteShader(frag_id);
    return id;
cleanup:
    glDeleteShader(vert_id);
    glDeleteShader(frag_id);
    glDeleteProgram(id);
    return 0;
}

#define FNV_PRIME 1099511628211ULL
#define FNV_SEED 1469598103934665603ULL
u64 private(fnv_1a)(const char* ptr, const u64 size) {
    u64 hash = FNV_SEED;
    u64 i = 0;
    for (; i + 4 < (u64)size; i += 4) {
        hash = (hash ^ ptr[i + 0]) * FNV_PRIME;
        hash = (hash ^ ptr[i + 1]) * FNV_PRIME;
        hash = (hash ^ ptr[i + 2]) * FNV_PRIME;
        hash = (hash ^ ptr[i + 4]) * FNV_PRIME;
    }
    for (; i < (u64)size; i++) hash = (hash ^ ptr[i]) * FNV_PRIME;
    return hash;
}
#define LOAD_FACTOR 0.75
#define DEFAULT_CAPACITY 32
static bool private(new_uniform_hashmap)(uniform_hashmap_t* map) {
    buf_t buffer = {
        .size = sizeof(entry_t) * DEFAULT_CAPACITY,
        .tag = MEMTAG_SHADER
    };
    if (!new_buf(&buffer, true)) return false;
    map->entries = buffer.ptr;
    map->capacity = DEFAULT_CAPACITY;
    map->count = 0;
    return true;
}
static void private(del_uniform_hashmap)(uniform_hashmap_t* map) {
    if (map->entries) del_buf(&(buf_t){ .ptr = map->entries, .size = sizeof(entry_t) * map->capacity, .tag = MEMTAG_SHADER});
}
static bool private(resize_uniform_hashmap)(uniform_hashmap_t* map) {
    u32 tmp = 0;
    const u64 new_capacity = map->capacity << 1;
    const u64 module = new_capacity - 1;

    buf_t buffer = {
        .size = new_capacity * sizeof(uniform_t),
        .tag = MEMTAG_SHADER
    };
    if (!new_buf(&buffer, true)) return false;

    entry_t* new_entries = (entry_t*)buffer.ptr;
    for (u64 i = 0; i < map->capacity; i++) {
        const entry_t* old = &map->entries[i];
        if (!old->uniform.address) continue;

        uniform_t uniform = old->uniform;

        u32 probe = 0;
        u64 index = uniform.hash & module;
        while (new_entries[index].uniform.address) {
            entry_t* entry = &new_entries[index];
            if (probe > entry->psl) {
                uniform_t tmp_uniform = entry->uniform;
                entry->uniform = uniform;
                uniform = tmp_uniform;

                tmp = entry->psl;
                entry->psl = probe;
                probe = tmp;
            }

            index = (index + 1) & module;
            probe++;
            if (probe >= new_capacity) {
                del_buf(&(buf_t){ .ptr = new_entries, .size = sizeof(entry_t) * new_capacity, .tag = MEMTAG_SHADER});
                return false;
            }
        }

        new_entries[index].uniform = uniform;
        new_entries[index].psl  = probe;
    }
    del_buf(&(buf_t){ .ptr = map->entries, .size = sizeof(entry_t) * map->capacity, .tag = MEMTAG_SHADER});
    map->entries  = new_entries;
    map->capacity = new_capacity;
    return true;
}
static bool private(push_uniform_hashmap)(shader_t* shader, const char* name, const u8 length, const i32 location) {
    if (shader->uniforms->count >= (u64)((f64)shader->uniforms->capacity * LOAD_FACTOR) && !private(resize_uniform_hashmap)(shader->uniforms)) return false;

    u32 tmp = 0;
    const u64 module = shader->uniforms->capacity - 1; // truncates redundant reuse of sub instruction

    uniform_t uniform = {
        .hash = private(fnv_1a)(name, length),
        .location = location,
        .address = (uptr)shader
    };

    u32 probe = 0;
    u64 index = uniform.hash & module;

    entry_t* entry = &shader->uniforms->entries[index];
    while (entry->uniform.address) {
        if (probe > entry->psl) {
            uniform_t tmp_uniform = entry->uniform;
            entry->uniform = uniform;
            uniform = tmp_uniform;

            tmp = entry->psl;
            entry->psl = probe;
            probe = tmp;
        }

        index = (index + 1) & module;
        entry = &shader->uniforms->entries[index];
        probe++;
        if (probe >= shader->uniforms->capacity) return false;
    }
    entry->uniform = uniform;
    entry->psl = probe;
    shader->uniforms->count++;
    return true;
}
uniform_t* private(search_uniform_hashmap)(shader_t* shader, const char* name, const i32 length) {
    const u64 module = shader->uniforms->capacity - 1; // truncates redundant reuse of sub instruction
    const u64 hash = private(fnv_1a)(name, length);
    u64 probe = 0;
    u64 index = hash & module;

    entry_t* entry = &shader->uniforms->entries[index];
    while (entry->uniform.address) {
        if (probe > entry->psl) break;

        uniform_t* uniform = &entry->uniform;
        if (
            uniform->hash == hash &&
            uniform->address == (uptr)shader
        ) return uniform;

        index = (index + 1) & module;
        entry = &shader->uniforms->entries[index];
        probe++;
        if (probe >= shader->uniforms->capacity) break;
    }

    return NULL;
}

#define SHADER_DIR __DIR__"\\Shader\\"
static struct {
    char* vertex;
    char* fragment;
    char* tag;
} __shader_paths[__SHADER_TAG_COUNT__] = {
    {SHADER_DIR"__comp__.vert", SHADER_DIR"__comp__.frag", "COMP"},
    {SHADER_DIR"__text__.vert", SHADER_DIR"__text__.frag", "TEXT"},
    {SHADER_DIR"__rect__.vert", SHADER_DIR"__rect__.frag", "RECT"},
};
shader_t* Shader(get)(frame_t* frame, const shader_tag_t tag) {
    shader_t* shader = &frame->shaders.cache[tag];
    return shader;
}
bool Shader(new_cache)(frame_t* frame) {
    printf(
"Jerking off shader\n"
       "------------------\n"
    );
    for (u32 tag = 0; tag < __SHADER_TAG_COUNT__; tag++) {
        shader_t* shader = &frame->shaders.cache[tag];

        const char* vertex_path = __shader_paths[tag].vertex;
        const char* fragment_path = __shader_paths[tag].fragment;
        const char* tag_label = __shader_paths[tag].tag;
        if (frame->shaders.uniforms.entries == NULL && !private(new_uniform_hashmap)(&frame->shaders.uniforms)) return false;
        shader->id = private(link_shaders)(vertex_path, fragment_path);
        if (shader->id == 0) {
            private(del_uniform_hashmap)(&frame->shaders.uniforms);
            for (u32 i = 0; i < tag; i++) glDeleteProgram(frame->shaders.cache[i].id);
            logFatal("Shader(new) - Failed to jerk off shader.");
            return false;
        }
        glUseProgram(shader->id);
        shader->uniforms = &frame->shaders.uniforms;
        printf("[%d/%d] Jerked off %s shader\n", tag + 1, __SHADER_TAG_COUNT__, tag_label);
    }
    return true;
}
void Shader(del_cache)(frame_t* frame) {
    private(del_uniform_hashmap)(&frame->shaders.uniforms);
    for (u8 i = 0; i < __SHADER_TAG_COUNT__; i++) {
        const u32 id = frame->shaders.cache[i].id;
        if (id) glDeleteProgram(id);
    }
}
static i32 private(get_uniform)(shader_t* shader, const char* name) {
    const u64 length = strlen(name);
    uniform_t* uniform = private(search_uniform_hashmap)(shader, name, length);

    i32 location = 0;
    if (uniform == NULL) {
        location = glGetUniformLocation(shader->id, name);
        if (location == -1) {
            logFatal("private(get_uniform) - Failed to find uniform: %s.", name);
            return -1;
        }

        if (!private(push_uniform_hashmap)(shader, name, length, location)) return -1;
    }
    else location = uniform->location;
    return location;
}
bool Shader(set_mat4_array)(shader_t* shader, const char* name, const u32 count, const bool transpose, const f32* elements) {
    const i32 location = private(get_uniform)(shader, name);
    if (location == -1) return false;

    glcall(glUniformMatrix4fv(location, count, transpose, elements), cleanup, "Shader(set_mat4_array) - Failed to set matrix uniform.");
    return true;
cleanup:
    return false;
}

bool Shader(set_float)(shader_t* shader, const char* name, const f32 value) {
    const i32 location = private(get_uniform)(shader, name);
    if (location == -1) return false;

    glcall(glUniform1f(location, value), cleanup, "Shader(set_float) - Failed to set float uniform.");
    return true;
cleanup:
    return false;
}

bool Shader(set_vec2_array)(shader_t* shader, const char* name, const u32 count, const f32* elements) {
    const i32 location = private(get_uniform)(shader, name);
    if (location == -1) return false;

    glcall(glUniform2fv(location, count, elements), cleanup, "Shader(set_vec2_array) - Failed to set vector uniform.");
    return true;
cleanup:
    return false;
}

bool Shader(set_vec4_array)(shader_t* shader, const char* name, const u32 count, const f32* elements) {
    const i32 location = private(get_uniform)(shader, name);
    if (location == -1) return false;

    glcall(glUniform4fv(location, count, elements), cleanup, "Shader(set_vec2_array) - Failed to set vector uniform.");
    return true;
cleanup:
    return false;
}