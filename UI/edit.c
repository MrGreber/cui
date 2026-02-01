#include <edit.h>
#include <mem.h>
#include <event_system.h>
#include <math-utils.h>
#include <frame.h>
#include <log.h>

#include <string.h>
#include <stdlib.h>
#include <memory.h>
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

__forceinline font_type_t __get_font_type(const char* font_name) {
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
    fread(&info, sizeof(struct fnt_info), 1, stream);

    u32 length = info.block_size - 14;
    byte* font_name = malloc(sizeof(byte) * (length + 1));
    if (font_name == NULL) {
        fclose(stream);
        return false;
    }
    fread(font_name, sizeof(byte), length, stream);
    font_name[length] = 0;

    fread(&common, sizeof(struct fnt_common), 1, stream);
    font->type = __get_font_type((const char*)font_name);
    font->size = info.font_size;
    font->line_height = common.lineHeight;
    *(u32*)&font->padding = *(u32*)&info.padding;
    const f32 inv_atlas_w = 1.0f / (f32)common.scale.width;
    const f32 inv_atlas_h = 1.0f / (f32)common.scale.height;
    free(font_name);

    fread(&pages, sizeof(struct fnt_pages), 1, stream);
    length = pages.block_size;
    const u64 dir_size = sizeof(__DIR__"/Resources/") - 1;
    byte* pages_name = malloc(sizeof(byte) * (dir_size + length + 1));
    if (pages_name == NULL) {
        fclose(stream);
        return false;
    }
    fread((char*)(pages_name + dir_size), sizeof(byte), length, stream);
    pages_name[length + dir_size] = 0;
    memcpy(pages_name, __DIR__"/Resources/", dir_size);
    const style_t style = {
        .init = true,
        .background = {
            .type = BG_IMAGE,
            .image = (const char*)pages_name
        },
    };
    if (!gen_texture(&font->atlas, NULL, &style)) return false;
    free(pages_name);
    fread(&chars, sizeof(struct fnt_chars), 1, stream);

    glyph_t* glyph = NULL;
    struct fnt_char char_ = { 0 };
    for (u32 i = 0; i < chars.block_size / (u32)sizeof(struct fnt_char); i++) {
        fread(&char_, sizeof(struct fnt_char), 1, stream);
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
__forceinline u16 __vcr_osd_mono_map(const char c) {
    if (c >= ' ' && c <= '~') return 2 + c - ' ';
    return 0;
}
static char_t __map_key(const char_t c, const bool is_shift) {
    static const char_t __map[] = {')', '!', '@', '#', '$', '%', '^', '&', '*', '('};

    if (is_shift) {
        if (c >= _C_'0' && c <= _C_'9') return __map[c - '0'];
        switch (c) {
            case '-': return '_';
            case '=': return '+';
            case '`': return '~';
            case ',': return '<';
            case '.': return '>';
            case '/': return '?';
            case ';': return ':';
            case '\'': return '\"';
            case '\\': return '|';
            case '[': return '{';
            case ']': return '}';
            default: return c;
        }
    }
    if (c >= _C_'A' && c <= _C_'Z') return c + 32;
    return c;
}

static font_t* new_font(const char* path) {
    buf_t buffer = {
        .size = sizeof(font_t),
        .tag = MEMTAG_FONT
    };
    if (!new_buf(&buffer, false)) return NULL;
    font_t* font = buffer.ptr;

    if (!__parse_fnt(path, font)) goto cleanup;
    buffer = (buf_t){
        .ptr = NULL,
        .size = QUAD_SIZE * DEFAULT_CAPACITY,
        .tag = MEMTAG_VECTOR
    };
    if (!new_buf(&buffer, true)) goto cleanup;
    font->mesh.vertices = buffer.ptr;
    font->mesh.capacity = DEFAULT_CAPACITY;
    font->mesh.count = 0;
    font->bg = TRANSP;
    font->fg = BLACK;

    font->mesh.va = new_vertex_array(2);
    font->mesh.vb = new_vertex_buffer(NULL, QUAD_SIZE * DEFAULT_CAPACITY, DYNAMIC_BUFFER);
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
    if (font->mesh.vertices) del_buf(&(buf_t){.ptr = font->mesh.vertices, .size = QUAD_SIZE * DEFAULT_CAPACITY, .tag = MEMTAG_VECTOR});
    del_buf(&(buf_t){.ptr = font, .size = sizeof(font_t), .tag = MEMTAG_FONT});
    return NULL;
}
static void del_font(font_t* font) {
    if (!font) return;
    del_texture(font->atlas);
    del_shader(font->shader);
    del_vertex_array(font->mesh.va);
    del_vertex_buffer(font->mesh.vb);
    del_buf(&(buf_t){.ptr = font->mesh.vertices, .size = QUAD_SIZE * font->mesh.capacity, .tag = MEMTAG_VECTOR});
    del_buf(&(buf_t){.ptr = font, .size = sizeof(font_t), .tag = MEMTAG_FONT});
}
static void bind_font(const font_t* font) {
    bind_vertex_array(font->mesh.va);
    glUseProgram(font->shader->id);
    bind_texture(font->atlas);
}
static bool __resize_text_mesh(font_t* font) {
    if (font->mesh.capacity == UINT64_MAX) {
        logError("__resize_text_mesh - Failed to resize text mesh, mesh reached max size %d.", UINT16_MAX);
        return false;
    }

    buf_t buffer = {
        .ptr = font->mesh.vertices,
        .size = QUAD_SIZE * font->mesh.capacity,
        .tag = MEMTAG_VECTOR
    };
    const u64 new_cap = font->mesh.capacity << 1;
    if (!renew_buf(&buffer, QUAD_SIZE * new_cap)) return false;
    font->mesh.vertices = buffer.ptr;
    font->mesh.capacity = new_cap;
    return true;
}

static void push_glyph_quad(font_t* font, const glyph_t* g, const f32 pen_x, const f32 pen_y) {
    if (font->mesh.capacity <= font->mesh.count && !__resize_text_mesh(font)) goto cleanup;

    vec4* ptr = font->mesh.vertices;
    const f32 x0 = pen_x + g->offset.x;
    const f32 y0 = pen_y + g->offset.y;
    const f32 x1 = x0 + g->dim.width;
    const f32 y1 = y0 + g->dim.height;

    const vec4 quad[6] = {
        {x0, y1, g->x0, g->y1},
        {x1, y0, g->x1, g->y0},
        {x0, y0, g->x0, g->y0},
        {x0, y1, g->x0, g->y1},
        {x1, y1, g->x1, g->y1},
        {x1, y0, g->x1, g->y0}
    };

    memcpy(&ptr[6 * font->mesh.count], quad, QUAD_SIZE);
    font->mesh.count++;
    return;
cleanup:
    logError("push_quad - Failed to resize text mesh.");
}
static void build_text_mesh(font_t* font, const void* text, f32 start_x, f32 start_y) {
    // TODO: optimize this function,
    // every AI I know of is dumb enough to not understand how to do it even though
    // the optimization is hella simple I mean I tried to do it myself for 2 times in a row
    // however it failed but I got close since I was able to render the text mesh semi correct and I know how to optimize
    // so i will try again sometime

    const struct {
        str_t* buffer;
        u64 index;
    }* edit_text = text;
    const str_t* buffer = edit_text->buffer;

    struct {
        vec2 pos;
        glyph_t* glyph;
    } caret = {
        .glyph = &font->table[__vcr_osd_mono_map('|')]
    };
    font->mesh.count = 0;

    vec2 pen = {
        start_x,
        start_y
    };

    bind_font(font);
    const f32 space_x = font->table[__vcr_osd_mono_map(' ')].x_advance;
    for (u64 i = 0; i < buffer->length; i++) {
        const char c = buffer->data[i];

        if (c == '\n') {
            pen.x = start_x;
            pen.y += font->line_height;
            if (edit_text->index == i + 1) {
                caret.pos = pen;
            }
            continue;
        }
        if (c == ' ') {
            pen.x += space_x;
            if (edit_text->index == i + 1) {
                caret.pos = pen;
            }
            continue;
        }
        if (c == '\t') {
            pen.x += 4.0f * space_x;
            if (edit_text->index == i + 1) {
                caret.pos = pen;
            }
            continue;
        }

        const u64 index = __vcr_osd_mono_map(c);
        const glyph_t* g = &font->table[index];

        push_glyph_quad(font, g, pen.x, pen.y);
        pen.x += g->x_advance;

        if (edit_text->index == i + 1) {
            caret.pos = pen;
        }
    }
    push_glyph_quad(font, caret.glyph, caret.pos.x - (f32)caret.glyph->offset.x, caret.pos.y);

    glBindBuffer(GL_ARRAY_BUFFER, font->mesh.vb->id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, font->mesh.count * QUAD_SIZE, font->mesh.vertices);
}

static void __default_mouse_callback(const mouse_cb_param* param) {
    const edit_t* edit = param->instance;
    const frame_t* frame = ((comp_node_t*)edit->header.components)->root->component.data;
}
static void __default_keyboard_callback(const keyboard_cb_param* param) {
    edit_t* edit = param->instance;
    frame_t* frame = ((comp_node_t*)edit->header.components)->root->component.data;

    bounding_box* box = &edit->header.box;
    if (param->action == GLFW_PRESS || param->action == GLFW_REPEAT) {
        switch (param->key) {
            case GLFW_KEY_ENTER: {
                insert_char(edit->text.buffer, edit->text.index++, _C_'\n');
                goto rebuild_text_mesh;
            }
            case GLFW_KEY_TAB: {
                insert_char(edit->text.buffer, edit->text.index++, _C_'\t');
                goto rebuild_text_mesh;
            }
            default: {
                const char_t key = __map_key(param->key, param->modes == GLFW_MOD_SHIFT);
                insert_char(edit->text.buffer, edit->text.index++, key);
                goto rebuild_text_mesh;
            }
            case GLFW_KEY_LEFT_CONTROL:
            case GLFW_KEY_RIGHT_CONTROL:
            case GLFW_KEY_ESCAPE:
            case GLFW_KEY_LEFT_SHIFT:
            case GLFW_KEY_RIGHT_SHIFT: return;
            case GLFW_KEY_HOME: {
                edit->text.index = rfind_char(edit->text.buffer, edit->text.index, '\n');
                goto rebuild_text_mesh;
            }
            case GLFW_KEY_END: {
                edit->text.index = find_char(edit->text.buffer, edit->text.index, '\n');
                goto rebuild_text_mesh;
            }
            case GLFW_KEY_LEFT: {
                if (edit->text.index) {
                    edit->text.index--;
                    goto rebuild_text_mesh;
                }
                break;
            }
            case GLFW_KEY_RIGHT: {
                if (edit->text.index < edit->text.buffer->length) {
                    edit->text.index++;
                    goto rebuild_text_mesh;
                }
                break;
            }
            case GLFW_KEY_UP: {
            }
            case GLFW_KEY_DOWN: {
                goto rebuild_text_mesh;
            }
            case GLFW_KEY_BACKSPACE: {
                if (edit->text.index) {
                    pop_char(edit->text.buffer, --edit->text.index);
                    goto rebuild_text_mesh;
                }
                break;
            }
        }
    }
    return;

rebuild_text_mesh:
    if (edit->font) {
        build_text_mesh(edit->font, &edit->text, 0.0, 0.0);
    }
}
static void __default_resize_callback(const resize_cb_param* param) {
    edit_t* edit = param->instance;
    edit->transform.init |= 1;
    // comp_header_t* header = get_header(edit->parent);
    // edit->header.box.width += param->width;
    // edit->header.box.height += param->height;
}

edit_t* new_edit(void* parent, const style_group_t* group, const bounding_box* box) {
    buf_t buffer = {
        .size = sizeof(edit_t),
        .tag = MEMTAG_EDIT
    };
    if (!new_buf(&buffer, true)) return NULL;

    const comp_header_t* parent_header = get_header(parent);

    edit_t* edit = buffer.ptr;
    edit->transform.init = 1;
    edit->header.box.x = box->x + parent_header->box.x;
    edit->header.box.y = box->y + parent_header->box.y;
    edit->header.box.width = box->width;
    edit->header.box.height = box->height;
    edit->parent = parent;
    if (group->normal.init) memcpy(&edit->styles.normal, &group->normal, sizeof(style_t));
    if (group->hover.init) memcpy(&edit->styles.hover, &group->hover, sizeof(style_t));

    edit->sprite = new_sprite("__component__");
    if (!edit->sprite) goto cleanup;
    if (!set_sprite_texture(edit->sprite, box->width, box->height, (style_t*)&group->normal)) goto cleanup;

    // Loads default font
    edit->font = new_font(__DIR__"\\Resources\\vcr_osd_mono.fnt");
    if (!edit->font) {
        logError("new_edit - Failed to load font.");
        goto cleanup;
    }

    edit->text.buffer = new_str("", 0);
    if (!edit->text.buffer) goto cleanup;

    edit->header.mouse = (callback)__default_mouse_callback;
    edit->header.keyboard = (callback)__default_keyboard_callback;
    edit->header.resize = (callback)__default_resize_callback;
    push_comp_node(parent_header->components, edit, EDIT_COMPONENT);
    return edit;
cleanup:
    if (edit->sprite) del_sprite(edit->sprite);
    if (edit->text.buffer) del_str(edit->text.buffer);
    if (edit->font) del_font(edit->font);
    del_buf(&(buf_t){.size = sizeof(edit_t), .tag = MEMTAG_EDIT, .ptr = edit});
    return NULL;
}
void del_edit(edit_t* edit) {
    if (!edit) return;
    del_sprite(edit->sprite);
    del_str(edit->text.buffer);
    del_font(edit->font);
    del_buf(&(buf_t){.size = sizeof(edit_t), .tag = MEMTAG_EDIT, .ptr = edit});
}
void bind_edit(const edit_t* edit) {
    if (!edit) return;
    bind_sprite(edit->sprite);
}
void set_font(edit_t* edit, const char* path, const color_t fg, const color_t bg) {
    if (!edit) {
        logError("set_font - Invalid parameter edit, address %p edit.\n", NULL);
        goto exit_set_font;
    }
    if (!path) {
        logError("set_font - Invalid parameter path, address %p path.\n", NULL);
        goto exit_set_font;
    }
    if (edit->font) del_font(edit->font);

    edit->font = new_font(path);
    if (!edit->font) {
        logError("set_font - Failed to load font.");
        goto exit_set_font;
    }
    edit->font->bg = bg;
    edit->font->fg = fg;
 exit_set_font:;
}
void update_edit(edit_t* edit, const mat4* projection, const f32 angle) {
    if (!edit) return;
    const frame_t* frame = ((comp_node_t*)edit->header.components)->root->component.data;
    const font_t* font = edit->font;
    const style_t* style = &edit->styles.normal;
    const vec4 border_color = {(f32)style->border.color.r / 255.0f, (f32)style->border.color.g / 255.0f, (f32)style->border.color.b / 255.0f, (f32)style->border.color.a / 255.0f};
    const vec2 dim = {(f32)edit->header.box.width, (f32)edit->header.box.height};

    if (edit->transform.init & 1) {
        const mat4 rotation = m4_rotateZ(rad(angle));
        const mat4 scale = m4_scale((f32)edit->header.box.width, (f32)edit->header.box.height, 1.0f);
        const mat4 position = m4_transl((f32)edit->header.box.x, (f32)edit->header.box.y, 0.0f);
        const mat4 size = m4_transl((f32)edit->header.box.width * 0.5f, (f32)edit->header.box.height * 0.5f, 0.0f);
        const mat4 inv_size = m4_transl(-(f32)edit->header.box.width * 0.5f, -(f32)edit->header.box.height * 0.5f, 0.0f);

        edit->transform.model = m4_mul(&position, &size);
        edit->transform.model = m4_mul(&edit->transform.model, &rotation);
        edit->transform.model = m4_mul(&edit->transform.model, &inv_size);
        edit->transform.model = m4_mul(&edit->transform.model, &scale);
        edit->transform.init ^= 1;
    }

    set_mat4_uniform(edit->sprite->shader, "projection", true, projection->e);
    set_mat4_uniform(edit->sprite->shader, "model", true, edit->transform.model.e);
    set_float_uniform(edit->sprite->shader, "border.radius", style->border.radius);
    set_float_uniform(edit->sprite->shader, "border.thickness", style->border.thickness);
    set_vec4_uniform(edit->sprite->shader, "border.color", border_color.e);
    set_vec2_uniform(edit->sprite->shader, "size", dim.e);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // draw the text mesh
    const mat4 position = m4_transl((f32)edit->header.box.x + style->border.thickness, (f32)edit->header.box.y + style->border.thickness, 0.0f);
    const mat4 size = m4_scale(1.0f, 1.0f, 1.0f);
    const mat4 model = m4_mul(&position, &size);
    bind_font(edit->font);
    // ToDO: change this to work for a rotated edit
    glEnable(GL_SCISSOR_TEST);
    glScissor(
        edit->header.box.x , frame->header.box.height - edit->header.box.y - edit->header.box.height + style->border.thickness,
        edit->header.box.width - style->border.thickness, edit->header.box.height - style->border.thickness
    );
    set_mat4_uniform(edit->font->shader, "projection", true, projection->e);
    set_mat4_uniform(edit->font->shader, "model", true, model.e);
    set_vec4_uniform(edit->font->shader, "font.bg", color_v4(font->bg).e);
    set_vec4_uniform(edit->font->shader, "font.fg", color_v4(font->fg).e);
    glDrawArrays(GL_TRIANGLES, 0, 6 * edit->font->mesh.count);
    glDisable(GL_SCISSOR_TEST);
}
