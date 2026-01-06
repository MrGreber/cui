#include <edit.h>
#include <mem.h>
#include <event_system.h>
#include <math-utils.h>
#include <frame.h>

#include <winsock.h>
#pragma comment(lib, "Ws2_32.lib")
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

    fread_s(&chars, sizeof(struct fnt_chars), sizeof(struct fnt_chars), 1, stream);
    glyph_t* glyph = NULL;
    // ToDo: allocate font.table
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
    if (!__parse_fnt(path, font)) {
        del_buf(&(buf_t){.ptr = font, .size = sizeof(font_t), .tag = MEMTAG_FONT});
        return NULL;
    }
    font->va = new_vertex_array(128);
    // ToDo: add vertex buffer allocation per char and load the texture.

    return font;
}
void del_font(font_t* font) {
    if (!font) return;
    del_vertex_array(font->va);
    del_buf(&(buf_t){.ptr = font, .size = sizeof(font_t), .tag = MEMTAG_FONT});
}

static void __default_mouse_callback(const mouse_cb_param* param) {
    edit_t* edit = param->instance;
    frame_t* frame = ((comp_node_t*)edit->header.components)->root->component.data;
    //printf("edit=%p\n", edit);
}

static void __default_keyboard_callback(const keyboard_cb_param* param) {
    edit_t* edit = param->instance;
    frame_t* frame = ((comp_node_t*)edit->header.components)->root->component.data;

    if (param->action == GLFW_PRESS || param->action == GLFW_REPEAT) {
        switch (param->key) {
            case GLFW_KEY_ENTER: {
                insert_char(edit->text, edit->index++, _C_'\n');
                break;
            }
            case GLFW_KEY_TAB: {
                insert_char(edit->text, edit->index++, _C_'\t');
                break;
            }
            default: {
                const u8 shift = param->modes == GLFW_MOD_SHIFT ? 0 : 32;
                const char_t key = (
                    param->key >= _C_'A' &&
                    param->key <= _C_'Z'
                ) ? param->key + shift : param->key;

                insert_char(edit->text, edit->index++, key);
                break;
            }
            case GLFW_KEY_LEFT_SHIFT:
            case GLFW_KEY_RIGHT_SHIFT: return;
            case GLFW_KEY_HOME: {
                edit->index = rfind_char(edit->text, edit->index, '\n');
                break;
            }
            case GLFW_KEY_END: {
                edit->index = find_char(edit->text, edit->index, '\n');
                break;
            }
            case GLFW_KEY_LEFT: {
                if (edit->index) {
                    edit->index--;
                }
                break;
            }
            case GLFW_KEY_RIGHT: {
                if (edit->index < edit->text->length) {
                    edit->index++;
                }
                break;
            }
            case GLFW_KEY_BACKSPACE: {
                if (edit->index) {
                    pop_char(edit->text, edit->index - 1);
                    edit->index--;
                }
                break;
            }

        }
        prints(edit->text, true);
        //printf("%c:%s\n", param->key, shift);
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

    font_t* font = new_font("C:\\Users\\roygr\\Downloads\\vcr_osd_mono\\vcr_osd_mono.bin");
    del_font(font);

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

    // style_t* normal_style = NULL,* hover_style = NULL;
    // if (group->normal.init) normal_style = &group->normal;
    // if (group->hover.init) hover_style = &group->hover;

    edit->sprite = new_sprite("__component__");
    if (!edit->sprite) goto cleanup;

    edit->text = new_str("", 0);
    if (!edit->text) goto cleanup;

    edit->header.mouse = __default_mouse_callback;
    edit->header.keyboard = __default_keyboard_callback;
    edit->header.resize =  __default_resize_callback;

    push_comp_node(parent_header->components, edit, PANEL_COMPONENT);
    return edit;
cleanup:
    if (edit->sprite) del_sprite(edit->sprite);
    if (edit->tex) del_texture(edit->tex);
    if (edit->text) del_str(edit->text);
    del_buf(&(buf_t){.size = sizeof(edit_t), .tag = MEMTAG_EDIT, .ptr = edit});
    return NULL;
}
void del_edit(edit_t* edit) {
    if (!edit) return;
    if (edit->sprite) del_sprite(edit->sprite);
    if (edit->tex) del_texture(edit->tex);

    prints(edit->text, true);
    if (edit->text) del_str(edit->text);
    del_buf(&(buf_t){.size = sizeof(edit_t), .tag = MEMTAG_EDIT, .ptr = edit});
}
void bind_edit(const edit_t* edit) {
    if (!edit) return;
    bind_sprite(edit->sprite);
    bind_texture(edit->tex);
}
void update_edit(edit_t* edit, const mat4* projection, const f32 angle) {
    if (!edit) return;

    const mat4 rotation = m4_rotateZ(rad(angle));
    const mat4 scale = m4_scale((f32)edit->header.box.width, (f32)edit->header.box.height, 1.0f);
    const mat4 position = m4_transl((f32)edit->header.box.x, (f32)edit->header.box.y, 0.0f);
    const mat4 size = m4_transl((f32)edit->header.box.width * 0.5f, (f32)edit->header.box.height * 0.5f, 0.0f);
    const mat4 inv_size = m4_transl(-(f32)edit->header.box.width * 0.5f, -(f32)edit->header.box.height * 0.5f, 0.0f);

    mat4 model = m4_mul(&position, &size);
    model = m4_mul(&model, &rotation);
    model = m4_mul(&model, &inv_size);
    model = m4_mul(&model, &scale);
    set_mat4_uniform(edit->sprite->shader, "projection", true, projection->e);
    set_mat4_uniform(edit->sprite->shader, "model", true, model.e);

    const style_t* style = &edit->styles.normal;
    const color_t border_color = style->border.color;
    const vec4 color = {(f32)border_color.r / 255.0f, (f32)border_color.g / 255.0f, (f32)border_color.b / 255.0f, (f32)border_color.a / 255.0f};
    const vec2 dim = {(f32)edit->header.box.width, (f32)edit->header.box.height};
    set_float_uniform(edit->sprite->shader, "border.radius", style->border.radius);
    set_float_uniform(edit->sprite->shader, "border.thickness", style->border.thickness);
    set_vec4_uniform(edit->sprite->shader, "border.color", color.e);
    set_vec2_uniform(edit->sprite->shader, "size", dim.e);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
