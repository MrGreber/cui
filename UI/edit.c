#include <edit.h>
#include <mem.h>
#include <event_system.h>
#include <math-utils.h>
#include <frame.h>
#include <log.h>

#include <string.h>
#include <stdlib.h>
#include <corecrt_memcpy_s.h>
#include <glad.h>
#include <glfw3.h>

#pragma pack(push, 1)
struct fnt_info {
    byte format[3];
    u8 version;
    u8 block_id;
    u32 block_size;

    u16 font_size;
    u8 bitField;
    u8 charSet;
    u16 stretchH;
    u8 aa;
    struct {
        u8 up;
        u8 right;
        u8 down;
        u8 left;
    } padding;
    struct {
        u8 horiz;
        u8 vert;
    } spacing;
    u8 outline;
};
struct fnt_common {
    u8 block_id;
    u32 block_size;

    u16 lineHeight;
    u16 base;
    struct {
        u16 width;
        u16 height;
    } scale;
    u16 pages;
    u8 bitField;
    struct {
        u8 a;
        u8 r;
        u8 g;
        u8 b;
    } color;
};
struct fnt_pages {
    u8 block_id;
    u32 block_size;
};
struct fnt_chars {
    u8 block_id;
    u32 block_size;
};
struct fnt_char {
    u32 id;
    u16 x;
    u16 y;
    struct {
        u16 width;
        u16 height;
    } dim;
    struct {
        u16 x;
        u16 y;
    } offset;
    u16 x_advance;
    u8 page;
    u8 channel;
};
#pragma pack(pop)
#define DEFAULT_CAPACITY 128
#define QUAD_SIZE (6 * sizeof(vec4))

static __forceinline font_type_t __get_font_type(const char* font_name) {
    if (strncmp(font_name, "VCR OSD Mono", 12) == 0) return VCR_OSD_MONO;
    else return 0;
}
static bool __parse_fnt(const char* path, font_t* font) {
    if (!font) return false;

    FILE* stream = NULL;
    if (fopen_s(&stream, path, "rb")) return false;

    struct fnt_info info = { 0 };
    struct fnt_common common = { 0 };
    struct fnt_pages pages = { 0 };
    struct fnt_chars chars = { 0 };
    fread_s(&info, sizeof(struct fnt_info), sizeof(struct fnt_info), 1, stream);

    u32 length = info.block_size - 14;
    byte* font_name = malloc(sizeof(byte) * (length + 1));
    if (font_name == NULL) {
        fclose(stream);
        return false;
    }
    fread_s(font_name, length + 1, sizeof(byte), length, stream);
    font_name[length] = 0;

    fread_s(&common, sizeof(struct fnt_common), sizeof(struct fnt_common), 1, stream);
    font->type = __get_font_type((const char*)font_name);
    font->size = info.font_size;
    font->line_height = common.lineHeight;
    *(u32*)&font->padding = *(u32*)&info.padding;
    const f32 inv_atlas_w = 1.0f / (f32)common.scale.width;
    const f32 inv_atlas_h = 1.0f / (f32)common.scale.height;
    free(font_name);

    fread_s(&pages, sizeof(struct fnt_pages), sizeof(struct fnt_pages), 1, stream);
    length = pages.block_size - 6;
    byte* pages_name = malloc(sizeof(byte) * (length + 1));
    if (pages_name == NULL) {
        fclose(stream);
        return false;
    }
    fread_s(pages_name, length + 1, sizeof(byte), length, stream);
    pages_name[length] = 0;
    free(pages_name);

    style_t style = {
        .init = true,
        .background = {
            .type = BG_IMAGE,
            .image = "C:\\Users\\roygr\\CLionProjects\\stream-draw\\Resources\\temp-char.png"
        },
    };
    if (!gen_comp_texture(&font->atlas,  &(bounding_box){0, 0, 16, 16}, &style)) return false;

    fread_s(&chars, sizeof(struct fnt_chars), sizeof(struct fnt_chars), 1, stream);
    glyph_t* glyph = NULL;
    struct fnt_char char_ = { 0 };
    for (u32 i = 0; i < chars.block_size / (u32)sizeof(struct fnt_char); i++) {
        fread_s(&char_, sizeof(struct fnt_char), sizeof(struct fnt_char), 1, stream);
        glyph = &font->table[i];
        glyph->id = char_.id;
        glyph->x0 = ((f32)char_.x) * inv_atlas_w;
        glyph->y0 = ((f32)char_.y) * inv_atlas_h;
        glyph->x1 = ((f32)(char_.x + char_.dim.width)) * inv_atlas_w;
        glyph->y1 = ((f32)(char_.y + char_.dim.height)) * inv_atlas_h;
        *(u32*)&glyph->dim =  *(u32*)&char_.dim;
        *(u32*)&glyph->offset =  *(u32*)&char_.offset;
        glyph->x_advance = char_.x_advance;
    }
    return true;
}

font_t* new_font(const char* path) {
    if (!path) return NULL;

    buf_t buffer = {
        .size = sizeof(font_t),
        .tag = MEMTAG_FONT
    };
    if (!new_buf(&buffer, false)) return NULL;
    font_t* font = buffer.ptr;

    if (!__parse_fnt(path, font)) goto cleanup;
    buffer = (buf_t){
        .ptr = NULL,
        .size = sizeof(vec4) * DEFAULT_CAPACITY,
        .tag = MEMTAG_VECTOR
    };
    if (!new_buf(&buffer, false)) goto cleanup;
    font->mesh.vertices = buffer.ptr;
    font->mesh.capacity = DEFAULT_CAPACITY;
    font->mesh.count = 0;
    font->size = 16;

    font->mesh.va = new_vertex_array(2);
    font->mesh.vb = new_vertex_buffer(NULL, 6 * sizeof(vec4) * DEFAULT_CAPACITY, DYNAMIC_BUFFER);
    if (!font->mesh.va || !font->mesh.vb) goto cleanup;
    bind_vertex_array(font->mesh.va);
    bind_vertex_buffer(font->mesh.vb);

    push_f32(font->mesh.va, 2);
    push_f32(font->mesh.va, 2);
    push_buf(font->mesh.va, font->mesh.vb);

    font->shader = new_shader("__text__");
    if (!font->shader) goto cleanup;
    return font;
cleanup:
    if (font->mesh.va) del_vertex_array(font->mesh.va);
    if (font->mesh.vb) del_vertex_buffer(font->mesh.vb);
    if (font->shader) del_shader(font->shader);
    if (font->atlas) del_texture(font->atlas);
    if (font->mesh.vertices) del_buf(&(buf_t){.ptr = font->mesh.vertices, .size = sizeof(vec4) * DEFAULT_CAPACITY, .tag = MEMTAG_VECTOR});
    del_buf(&(buf_t){.ptr = font, .size = sizeof(font_t), .tag = MEMTAG_FONT});
    return NULL;
}
void del_font(font_t* font) {
    if (!font) return;
    del_texture(font->atlas);
    del_shader(font->shader);
    del_vertex_array(font->mesh.va);
    del_vertex_buffer(font->mesh.vb);
    del_buf(&(buf_t){.ptr = font->mesh.vertices, .size = sizeof(vec4) * font->mesh.capacity, .tag = MEMTAG_VECTOR});
    del_buf(&(buf_t){.ptr = font, .size = sizeof(font_t), .tag = MEMTAG_FONT});
}
void bind_font(const font_t* font) {
    bind_vertex_array(font->mesh.va);
    glUseProgram(font->shader->id);
    bind_texture(font->atlas);
}
bool __resize_text_mesh(font_t* font) {
    if (font->mesh.capacity == UINT64_MAX) {
        logError("__resize_text_mesh - Failed to resize text mesh, mesh reached max size %d.", UINT16_MAX);
        return false;
    }

    buf_t buffer = {
        .ptr = font->mesh.vertices,
        .size = sizeof(vec4) * font->mesh.capacity,
        .tag = MEMTAG_BYTE
    };
    const u64 new_cap = font->mesh.capacity << 1;
    if (!renew_buf(&buffer, sizeof(vec4) * new_cap)) return false;
    font->mesh.vertices = buffer.ptr;
    font->mesh.capacity = new_cap;
    return true;
}
void push_quad(font_t* font, const vec4* quad) {
    if (font->mesh.capacity <= font->mesh.count && !__resize_text_mesh(font)) goto cleanup;

    vec4* ptr = font->mesh.vertices;
    const u64 i = font->mesh.count;
    const u64 size = font->mesh.count - i;

    memcpy(&ptr[i], quad, QUAD_SIZE);
    font->mesh.count += 6;

    bind_font(font);
    glBindBuffer(GL_ARRAY_BUFFER, font->mesh.vb->id);
    glBufferSubData(GL_ARRAY_BUFFER, i * sizeof(vec4), (size + 6) * sizeof(vec4), &ptr[i]);

    return;
cleanup:
    logError("push_quad - Failed to resize text mesh.");
}
void insert_quad(font_t* font, const u64 index, const vec4* quad) {
    if (font->mesh.capacity <= font->mesh.count && !__resize_text_mesh(font)) goto cleanup;

    vec4* ptr = font->mesh.vertices;
    const u64 i = 6 * index;
    const u64 size = font->mesh.count - i;

    memmove(&ptr[i + 6], &ptr[i], size * sizeof(vec4));
    memcpy(&ptr[i], quad, QUAD_SIZE);
    font->mesh.count += 6;

    bind_font(font);
    glBindBuffer(GL_ARRAY_BUFFER, font->mesh.vb->id);
    glBufferSubData(GL_ARRAY_BUFFER, i * sizeof(vec4), (size + 6) * sizeof(vec4), &ptr[i]);

    return;
cleanup:
    logError("insert_quad - Failed to resize text mesh.");
}
void pop_quad(font_t* font, const u64 index) {
    if (index * 6 >= font->mesh.count) return;

    vec4* ptr = font->mesh.vertices;
    const u64 i = 6 * index;
    const u64 size = font->mesh.count - (i + 6);

    memmove(&ptr[i], &ptr[i + 6], size * sizeof(vec4));
    font->mesh.count -= 6;

    bind_font(font);
    glBindBuffer(GL_ARRAY_BUFFER, font->mesh.vb->id);
    glBufferSubData(GL_ARRAY_BUFFER, i * sizeof(vec4), size * sizeof(vec4), &ptr[i]);
}

static void __default_mouse_callback(const mouse_cb_param* param) {
    edit_t* edit = param->instance;
    frame_t* frame = ((comp_node_t*)edit->header.components)->root->component.data;
    //printf("edit=%p\n", edit);
}
static void __default_keyboard_callback(const keyboard_cb_param* param) {
    edit_t* edit = param->instance;
    frame_t* frame = ((comp_node_t*)edit->header.components)->root->component.data;

    const f32 size = (f32)edit->font->size;
    const f32 offset_x = (f32)edit->text.index * size;
    if (param->action == GLFW_PRESS || param->action == GLFW_REPEAT) {
        switch (param->key) {
            case GLFW_KEY_ENTER: {
                insert_char(edit->text.buffer, edit->text.index++, _C_'\n');
                break;
            }
            case GLFW_KEY_TAB: {
                insert_char(edit->text.buffer, edit->text.index++, _C_'\t');
                break;
            }
            default: {
                const u8 shift = param->modes == GLFW_MOD_SHIFT ? 0 : 32;
                const char_t key = (
                    param->key >= _C_'A' &&
                    param->key <= _C_'Z'
                ) ? param->key + shift : param->key;

                insert_quad(edit->font, edit->text.index, (vec4[6]){
                    {offset_x, size, 0.0f, 1.0f},
                    {size + offset_x, 0.0f, 1.0f, 0.0f},
                    {offset_x, 0.0f, 0.0f, 0.0f},
                    {offset_x, size, 0.0f, 1.0f},
                    {size + offset_x, size, 1.0f, 1.0f},
                    {size + offset_x, 0.0f, 1.0f, 0.0f}
                });
                insert_char(edit->text.buffer, edit->text.index++, key);
                break;
            }
            case GLFW_KEY_LEFT_SHIFT:
            case GLFW_KEY_RIGHT_SHIFT: return;
            case GLFW_KEY_HOME: {
                edit->text.index = rfind_char(edit->text.buffer, edit->text.index, '\n');
                break;
            }
            case GLFW_KEY_END: {
                edit->text.index = find_char(edit->text.buffer, edit->text.index, '\n');
                break;
            }
            case GLFW_KEY_LEFT: {
                if (edit->text.index) {
                    edit->text.index--;
                }
                break;
            }
            case GLFW_KEY_RIGHT: {
                if (edit->text.index < edit->text.buffer->length) {
                    edit->text.index++;
                }
                break;
            }
            case GLFW_KEY_BACKSPACE: {
                if (edit->text.index) {
                    pop_quad(edit->font, --edit->text.index);
                    pop_char(edit->text.buffer, edit->text.index);
                }
                break;
            }

        }
        prints(edit->text.buffer, true);
    }
}
static void __default_resize_callback(const resize_cb_param* param) {
    edit_t* edit = param->instance;
    //comp_header_t* header = get_header(panel->parent);

    // panel->header.box.width += param->width;
    // panel->header.box.height += param->height;

}

edit_t* new_edit(void* parent, style_group_t* group, const bounding_box* box) {
    buf_t buffer = {
        .size = sizeof(edit_t),
        .tag = MEMTAG_EDIT
    };
    if (!new_buf(&buffer, true)) return NULL;

    const comp_header_t* parent_header = get_header(parent);

    edit_t* edit = buffer.ptr;
    edit->header.box.x = box->x + parent_header->box.x;
    edit->header.box.y = box->y + parent_header->box.y;
    edit->header.box.width = box->width;
    edit->header.box.height = box->height;
    edit->parent = parent;
    if (group->normal.init) memcpy_s(&edit->styles.normal, sizeof(style_t), &group->normal, sizeof(style_t));
    if (group->hover.init) memcpy_s(&edit->styles.hover, sizeof(style_t), &group->hover, sizeof(style_t));

    if (!gen_comp_texture(&edit->tex, box, &group->normal)) goto cleanup;

    edit->sprite = new_sprite("__component__");
    if (!edit->sprite) goto cleanup;

    edit->font = new_font("C:\\Users\\roygr\\Downloads\\vcr_osd_mono\\vcr_osd_mono.bin");
    if (!edit->font) {
        logError("new_edit - Failed to load font.");
        goto cleanup;
    }

    edit->text.buffer = new_str("", 0);
    if (!edit->text.buffer) goto cleanup;

    edit->header.mouse = __default_mouse_callback;
    edit->header.keyboard = __default_keyboard_callback;
    edit->header.resize =  __default_resize_callback;

    push_comp_node(parent_header->components, edit, PANEL_COMPONENT);
    return edit;
cleanup:
    if (edit->sprite) del_sprite(edit->sprite);
    if (edit->tex) del_texture(edit->tex);
    if (edit->text.buffer) del_str(edit->text.buffer);
    if (edit->font) del_font(edit->font);
    del_buf(&(buf_t){.size = sizeof(edit_t), .tag = MEMTAG_EDIT, .ptr = edit});
    return NULL;
}
void del_edit(edit_t* edit) {
    if (!edit) return;
    del_sprite(edit->sprite);
    del_texture(edit->tex);
    del_str(edit->text.buffer);
    del_font(edit->font);
    del_buf(&(buf_t){.size = sizeof(edit_t), .tag = MEMTAG_EDIT, .ptr = edit});
}
void bind_edit(const edit_t* edit) {
    if (!edit) return;
    bind_sprite(edit->sprite);
    bind_texture(edit->tex);
}
void update_edit(edit_t* edit, const mat4* projection, const f32 angle) {
    if (!edit) return;

    const style_t* style = &edit->styles.normal;
    const color_t border_color = style->border.color;
    vec4 color = {(f32)border_color.r / 255.0f, (f32)border_color.g / 255.0f, (f32)border_color.b / 255.0f, (f32)border_color.a / 255.0f};
    const vec2 dim = {(f32)edit->header.box.width, (f32)edit->header.box.height};

    mat4 rotation = m4_rotateZ(rad(angle));
    mat4 scale = m4_scale((f32)edit->header.box.width, (f32)edit->header.box.height, 1.0f);
    mat4 position = m4_transl((f32)edit->header.box.x, (f32)edit->header.box.y, 0.0f);
    mat4 size = m4_transl((f32)(edit->header.box.width - style->border.thickness) * 0.5f, (f32)(edit->header.box.height - style->border.thickness) * 0.5f, 0.0f);
    mat4 inv_size = m4_transl(-(f32)(edit->header.box.width - style->border.thickness) * 0.5f, -(f32)(edit->header.box.height - style->border.thickness) * 0.5f, 0.0f);

    mat4 model = m4_mul(&position, &size);
    model = m4_mul(&model, &rotation);
    model = m4_mul(&model, &inv_size);
    model = m4_mul(&model, &scale);
    set_mat4_uniform(edit->sprite->shader, "projection", true, projection->e);
    set_mat4_uniform(edit->sprite->shader, "model", true, model.e);
    set_float_uniform(edit->sprite->shader, "border.radius", style->border.radius);
    set_float_uniform(edit->sprite->shader, "border.thickness", style->border.thickness);
    set_vec4_uniform(edit->sprite->shader, "border.color", color.e);
    set_vec2_uniform(edit->sprite->shader, "size", dim.e);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // draw the text mesh
    position = m4_transl((f32)edit->header.box.x + style->border.thickness, (f32)edit->header.box.y + style->border.thickness, 0.0f);
    color = (vec4){1.0f, 0.0f, 0.0f, 1.0f};
    const vec4 bg = {0.0, 0.0, 0.0, 1.0f};
    bind_font(edit->font);
    set_mat4_uniform(edit->font->shader, "projection", true, projection->e);
    set_mat4_uniform(edit->font->shader, "model", true, position.e);
    set_vec4_uniform(edit->font->shader, "font.bg", bg.e);
    set_vec4_uniform(edit->font->shader, "font.fg", color.e);
    glDrawArrays(GL_TRIANGLES, 0, 6 * edit->text.buffer->length);
}
