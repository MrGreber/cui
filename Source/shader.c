#include <shader.h>
#include <memio.h>
#include <log.h>
#include <utils.h>

#include <glad.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

static bool __compile_shader(const u32 type, const char* path, u32* id) {
    char* source = NULL;
    u64 size = 0;
    if (!read_file(path, &source, &size)) {
        logFatal("__compile_shader - Failed to read shader.");
        return false;
    }

    const u32 _id = glCreateShader(type);
    if (!_id) goto cleanup;
    glcall(glShaderSource(_id, 1, (const GLchar**)&source, NULL), cleanup, "__compile_shader - Failed to build shader: %s.", path);
    glcall(glCompileShader(_id), cleanup, "__compile_shader - Failed to compile shader: %s.", path);

    i32 success = 0;
    glcall(glGetShaderiv(_id, GL_COMPILE_STATUS, &success), cleanup, "__compile_shader - Failed to get shader: %s iv.", path);
    if (!success) {
        char msg[512] = { 0 };
        glGetShaderInfoLog(_id, 512, NULL, msg);
        logError("__compile_shader - shader compilation error:\n%s", msg);
        goto cleanup;
    }

    *id = _id;
    del_buf(&(buf_t){.size = size, .tag = MEMTAG_BYTE, .ptr = source});
    return true;
cleanup:
    if (source) del_buf(&(buf_t){.size = size, .tag = MEMTAG_BYTE, .ptr = source});
    return false;
}
static u32 __link_shader_program(const char* vertex_path, const char* fragment_path, u32* id) {
    u32 vert_id = 0;
    u32 frag_id = 0;
    if (!__compile_shader(GL_VERTEX_SHADER, vertex_path, &vert_id) || !__compile_shader(GL_FRAGMENT_SHADER, fragment_path, &frag_id)) {
        logFatal("Failed to compile vertex/fragment shader.");
        goto cleanup;
    }

    const u32 _id = glCreateProgram();
    if (!_id) goto cleanup;
    glcall(glAttachShader(_id, vert_id), cleanup, "__create_shader_program - Failed to attach vertex shader.");
    glcall(glAttachShader(_id, frag_id), cleanup, "__create_shader_program - Failed to attach fragment shader.");
    glcall(glLinkProgram(_id), cleanup, "__create_shader_program - Failed to link shader.");

    i32 success = 0;
    glcall(glGetProgramiv(_id, GL_LINK_STATUS, &success), cleanup, "__compile_shader - Failed to get shader iv.");

    *id = _id;
    glDeleteShader(vert_id);
    glDeleteShader(frag_id);
    return true;
cleanup:
    glDeleteShader(vert_id);
    glDeleteShader(frag_id);
    glDeleteProgram(_id);
    return false;
}



#define UNIMAP_END UINT32_MAX
#define DEFAULT_CAPACITY 16
static u64 __unimap_hash_function(const char* var, const u8 length) {
    u64 hash = 1469598103934665603ULL;

    for (u16 i = 0; i < length && i < 8; i++) {
        hash ^= var[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}
static bool __new_uniform_map(shader_t* shader) {
    buf_t buffer = {
        .size = sizeof(unimap_t),
        .tag = MEMTAG_HASHMAP
    };
    if (!new_buf(&buffer, true)) goto cleanup;
    shader->map = buffer.ptr;

    buffer = (buf_t){
        .size = sizeof(uniform_t) * DEFAULT_CAPACITY,
        .tag = MEMTAG_KEY_VALUE_PAIR
    };
    if (!new_buf(&buffer, true)) goto cleanup;
    shader->map->elem = buffer.ptr;

    buffer = (buf_t){
        .size = sizeof(uniform_t) * DEFAULT_CAPACITY,
        .tag = MEMTAG_KEY_VALUE_PAIR
    };
    if (!new_buf(&buffer, true)) goto cleanup;
    shader->map->collisions.elem = buffer.ptr;

    shader->map->collisions.capacity = DEFAULT_CAPACITY;
    return true;
cleanup:
    if (shader->map->elem) del_buf(&(buf_t){.ptr = shader->map->elem, .size = sizeof(uniform_t) * DEFAULT_CAPACITY, .tag = MEMTAG_KEY_VALUE_PAIR});
    if (shader->map->collisions.elem) del_buf(&(buf_t){.ptr = shader->map->collisions.elem, .size = sizeof(uniform_t) * DEFAULT_CAPACITY, .tag = MEMTAG_KEY_VALUE_PAIR});
    if (shader->map) del_buf(&(buf_t){.ptr = shader->map, .size = sizeof(unimap_t), .tag = MEMTAG_HASHMAP});
    return false;
}
static bool __resize_unimap(unimap_t* map) {
    if (map->collisions.capacity >= UINT64_MAX) {
        logError("__resize_unimap - Failed to resize uniform map, uniform map collision array reached max size %d.", UINT64_MAX);
        return false;
    }

    const u64 new_capacity = map->collisions.capacity << 1;
    buf_t buffer = {
        .ptr = map->collisions.elem,
        .size = map->collisions.capacity * sizeof(uniform_t),
        .tag = MEMTAG_KEY_VALUE_PAIR
    };
    if (!renew_buf(&buffer, new_capacity * sizeof(uniform_t))) return false;
    map->collisions.elem =  buffer.ptr;
    map->collisions.capacity = new_capacity;

    return true;
}

static bool __insert_unimap(unimap_t* map, const char* name, const u8 length, const i32 location) {
    if (map->collisions.count == map->collisions.capacity && !__resize_unimap(map)) return false;

    const u8 min = length > MAX_UNIFORM_NAME ? MAX_UNIFORM_NAME : length;
    const u64 index = __unimap_hash_function(name, length) & (DEFAULT_CAPACITY - 1);
    uniform_t* dst = &map->elem[index];

    if (dst->name[0] == name[0] && !memcmp(dst->name, name, min)) dst->location = location;
    else if (!dst->name[0]) {
        memcpy(dst->name, name, min);
        dst->location = location;
        dst->next_index = UNIMAP_END;
    }
    else {
        uniform_t* cur = dst;
        while (
            cur->next_index != UNIMAP_END && // checks for initialization of the next node
            (cur->name[0] != name[0] || memcmp(cur->name, name, min) != 0)
        ) cur = &map->collisions.elem[cur->next_index];

        if (cur->name[0] != name[0] || memcmp(cur->name, name, min) != 0) {
            cur->next_index = map->collisions.count;
            uniform_t* next = &map->collisions.elem[map->collisions.count++];
            memcpy(next->name, name, min);
            next->location = location;
            next->next_index = UNIMAP_END;
        }
        else cur->location = location;
    }
    return true;
}
static i32 __search_unimap(unimap_t* map, const char* name, const u8 length) {
    if (!map || !name || !length) return -1;

    const u8 min = length > MAX_UNIFORM_NAME ? MAX_UNIFORM_NAME : length;
    const u64 index = __unimap_hash_function(name, length) & (map->collisions.capacity - 1);

    uniform_t* cur = &map->elem[index];
    if (!cur->name[0]) return -1;

    while (
        cur->next_index != UNIMAP_END && // checks for initialization of the next node
        (cur->name[0] != name[0] || memcmp(cur->name, name, min) != 0)
    ) cur = &map->collisions.elem[cur->next_index];

    if (cur->name[0] != name[0] || memcmp(cur->name, name, min) != 0) return -1;
    else return cur->location;
}

static void __del_unimap(unimap_t* map) {
    if (!map) return;
    del_buf(&(buf_t){.ptr = map->elem, .size = sizeof(uniform_t) * DEFAULT_CAPACITY, .tag = MEMTAG_KEY_VALUE_PAIR});
    del_buf(&(buf_t){.ptr = map->collisions.elem, .size = sizeof(uniform_t) * DEFAULT_CAPACITY, .tag = MEMTAG_KEY_VALUE_PAIR});
    del_buf(&(buf_t){.ptr = map, .size = sizeof(unimap_t), .tag = MEMTAG_HASHMAP});
}

static void __print_unimap(unimap_t* map) {
    if (!map) return;

    for (u64 i = 0; i < DEFAULT_CAPACITY; i++) {
        const uniform_t* uniform = &map->elem[i];
        if (uniform->name[0]) printf("\t%.32s: %d\n", uniform->name, uniform->location);
    }
    for (u64 i = 0; i < map->collisions.count; i++) {
        const uniform_t* uniform = &map->collisions.elem[i];
        if (uniform->name[0]) printf("\t[coll]%-.32s: %d\n", uniform->name, uniform->location);
    }
}

#define SHADER_DIR __DIR__"\\Shader\\"
shader_t* new_shader(const char* name) {
    buf_t buffer = {
        .size = sizeof(shader_t),
        .tag = MEMTAG_SHADER,
    };
    if (!new_buf(&buffer, false)) return NULL;

    shader_t* shad = buffer.ptr;

    char vertex_path[256] = { 0 };
    char fragment_path[256] = { 0 };
    sprintf_s(vertex_path, 256, SHADER_DIR"%s.vert", name);
    sprintf_s(fragment_path, 256, SHADER_DIR"%s.frag", name);
    if (!__new_uniform_map(shad)) {
        del_buf(&buffer);
        return NULL;
    }

    if (!__link_shader_program(
        vertex_path,
        fragment_path,
        &shad->id
    )) {
        logFatal("new_shader - Failed to compile shader.");
        del_buf(&buffer);
        __del_unimap(shad->map);
        return NULL;
    }
    glUseProgram(shad->id);
    return shad;
}
void del_shader(shader_t* shad) {
    if (!shad) return;
    glDeleteProgram(shad->id);
    __del_unimap(shad->map);
    del_buf(&(buf_t){.size = sizeof(shader_t), .tag = MEMTAG_SHADER, .ptr = shad});
}


static i32 __get_uniform_location(unimap_t* map, const u32 id, const char* name) {
    const u64 length = strlen(name);
    i32 location = __search_unimap(map, name, length);

    if (map && location == -1) {
        location = glGetUniformLocation(id, name);
        if (location == -1) {
            logFatal("__get_uniform_location - Failed to find uniform: %s.", name);
            return -1;
        }

        if (!__insert_unimap(map, name, length, location)) return -1;
    }

    return location;
}
bool set_mat4_uniform_array(const shader_t* shad, const char* name, const u32 count, const bool transpose, const f32* elements) {
    const i32 location = __get_uniform_location(shad->map, shad->id, name);
    if (location == -1) return false;

    glcall(glUniformMatrix4fv(location, count, transpose, elements), cleanup, "set_mat4_uniform_array - Failed to set matrix uniform.");
    return true;
cleanup:
    return false;
}
bool set_float_uniform(const shader_t* shad, const char* name, const f32 v) {
    const i32 location = __get_uniform_location(shad->map, shad->id, name);
    if (location == -1) return false;

    glcall(glUniform1f(location, v), cleanup, "set_float_uniform - Failed to set float uniform.");
    return true;
cleanup:
    return false;
}

bool set_vec2_uniform_array(const shader_t* shad, const char* name, const u32 count, const f32* elements) {
    const i32 location = __get_uniform_location(shad->map, shad->id, name);
    if (location == -1) return false;

    glcall(glUniform2fv(location, count, elements), cleanup, "set_vec2_uniform_array - Failed to set float uniform.");
    return true;
cleanup:
    return false;
}

bool set_vec4_uniform_array(const shader_t* shad, const char* name, const u32 count, const f32* elements) {
    const i32 location = __get_uniform_location(shad->map, shad->id, name);
    if (location == -1) return false;

    glcall(glUniform4fv(location, count, elements), cleanup, "set_vec4_uniform_array - Failed to set float uniform.");
    return true;
cleanup:
    return false;
}

void print_shader(shader_t* shad) {
    if (!shad) return;
    printf("shader[%d]:\n", shad->id);
    __print_unimap(shad->map);
}