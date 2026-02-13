#include <edit.h>
#include <mem.h>
#include <event_system.h>
#include <math-utils.h>
#include <frame.h>
#include <log.h>

#include <string.h>
#include <stdlib.h>
#include <glad.h>
#include <glfw3.h>

#define DEFAULT_CAPACITY 128
#define QUAD_SIZE (6 * sizeof(vec4))

static bool __resize_text_mesh(edit_t* edit) {
    if (edit->mesh.capacity == UINT64_MAX) {
        logError("__resize_text_mesh - Failed to resize text mesh, mesh reached max size %d.", UINT16_MAX);
        return false;
    }

    buf_t buffer = {
        .ptr = edit->mesh.vertices,
        .size = QUAD_SIZE * edit->mesh.capacity,
        .tag = MEMTAG_VECTOR
    };
    const u64 new_cap = edit->mesh.capacity << 1;
    if (!renew_buf(&buffer, QUAD_SIZE * new_cap)) return false;
    edit->mesh.vertices = buffer.ptr;
    edit->mesh.capacity = new_cap;
    return true;
}
static void push_glyph_quad(edit_t* edit, const glyph_t* g, const f32 pen_x, const f32 pen_y) {
    if (edit->mesh.capacity <= edit->mesh.count && !__resize_text_mesh(edit)) goto cleanup;

    vec4* ptr = edit->mesh.vertices;
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

    memcpy(&ptr[6 * edit->mesh.count], quad, QUAD_SIZE);
    edit->mesh.count++;
    return;
cleanup:
    logError("push_quad - Failed to resize text mesh.");
}
static void build_text_mesh(edit_t* edit, const void* text, f32 start_x, f32 start_y) {
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
        .glyph = &edit->font->glyphs[edit->font->amap('|')]
    };
    edit->mesh.count = 0;

    vec2 pen = {
        start_x,
        start_y
    };

    bind_vertex_array(edit->mesh.va);
    glUseProgram(edit->mesh.shader->id);
    bind_font(edit->font);
    const f32 space_x = edit->font->glyphs[edit->font->amap(' ')].x_advance;
    for (u64 i = 0; i < buffer->length; i++) {
        const char c = buffer->data[i];

        if (c == '\n') {
            pen.x = start_x;
            pen.y += edit->font->line_height;
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

        const u64 index = edit->font->amap(c);
        const glyph_t* g = &edit->font->glyphs[index];

        push_glyph_quad(edit, g, pen.x, pen.y);
        pen.x += g->x_advance;

        if (edit_text->index == i + 1) {
            caret.pos = pen;
        }
    }
    push_glyph_quad(edit, caret.glyph, caret.pos.x - (f32)caret.glyph->offset.x, caret.pos.y);

    glBindBuffer(GL_ARRAY_BUFFER, edit->mesh.vb->id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, edit->mesh.count * QUAD_SIZE, edit->mesh.vertices);
}

static void __default_mouse_callback(const mouse_cb_param* param) {
    edit_t* edit = param->instance;
    const frame_t* frame = ((comp_node_t*)edit->header.components)->root->component.data;

    if (param->action == GLFW_PRESS) {
        build_text_mesh(edit, &edit->text, 0.0, 0.0);
    }
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
                const char_t key = edit->font->kmap(param->key, param->modes == GLFW_MOD_SHIFT);
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
        build_text_mesh(edit, &edit->text, 0.0, 0.0);
    }
}
static void __default_resize_callback(const resize_cb_param* param) {
    edit_t* edit = param->instance;
    edit->transform.init |= 1;
    // comp_header_t* header = get_header(edit->parent);
    // edit->header.box.width += param->width;
    // edit->header.box.height += param->height;
}


void set_edit_text(edit_t* edit, char_t* text, const u64 length) {
    if (!assign_str(edit->text.buffer, text, length)) {
        logError("set_text - Failed to set edit, text.");
        return;
    }
    build_text_mesh(edit, &edit->text, 0.0, 0.0);
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

    buffer = (buf_t){
        .ptr = NULL,
        .size = QUAD_SIZE * DEFAULT_CAPACITY,
        .tag = MEMTAG_VECTOR
    };
    if (!new_buf(&buffer, true)) goto cleanup;
    edit->mesh.vertices = buffer.ptr;
    edit->mesh.capacity = DEFAULT_CAPACITY;
    edit->mesh.count = 0;

    edit->mesh.va = new_vertex_array(2);
    edit->mesh.vb = new_vertex_buffer(NULL, QUAD_SIZE * DEFAULT_CAPACITY, DYNAMIC_BUFFER);
    if (!edit->mesh.va || !edit->mesh.vb) goto cleanup;
    bind_vertex_array(edit->mesh.va);
    bind_vertex_buffer(edit->mesh.vb);

    push_f32(edit->mesh.va, 2);
    push_f32(edit->mesh.va, 2);
    push_buf(edit->mesh.va, edit->mesh.vb);

    edit->mesh.shader = new_shader("__text__");
    if (!edit->mesh.shader) goto cleanup;

    edit->text.buffer = new_str("", 0);
    if (!edit->text.buffer) goto cleanup;

    edit->header.mouse = (callback)__default_mouse_callback;
    edit->header.keyboard = (callback)__default_keyboard_callback;
    edit->header.resize = (callback)__default_resize_callback;
    push_comp_node(parent_header->components, edit, EDIT_COMPONENT);

    build_text_mesh(edit, &edit->text, 0.0, 0.0);
    return edit;
cleanup:
    if (edit->mesh.va) del_vertex_array(edit->mesh.va);
    if (edit->mesh.vb) del_vertex_buffer(edit->mesh.vb);
    if (edit->mesh.shader) del_shader(edit->mesh.shader);
    if (edit->mesh.vertices) del_buf(&(buf_t){.ptr = edit->mesh.vertices, .size = QUAD_SIZE * DEFAULT_CAPACITY, .tag = MEMTAG_VECTOR});
    if (edit->sprite) del_sprite(edit->sprite);
    if (edit->text.buffer) del_str(edit->text.buffer);
    if (edit->font) del_font(edit->font);
    del_buf(&(buf_t){.size = sizeof(edit_t), .tag = MEMTAG_EDIT, .ptr = edit});
    return NULL;
}
void del_edit(edit_t* edit) {
    if (!edit) return;
    del_vertex_array(edit->mesh.va);
    del_vertex_buffer(edit->mesh.vb);
    del_shader(edit->mesh.shader);
    del_buf(&(buf_t){.ptr = edit->mesh.vertices, .size = QUAD_SIZE * edit->mesh.capacity, .tag = MEMTAG_VECTOR});
    del_sprite(edit->sprite);
    del_str(edit->text.buffer);
    del_font(edit->font);
    del_buf(&(buf_t){.size = sizeof(edit_t), .tag = MEMTAG_EDIT, .ptr = edit});
}
void bind_edit(const edit_t* edit) {
    if (!edit) return;
    bind_sprite(edit->sprite);
}

#define CLOCK_TIME 0.02
void update_edit(edit_t* edit, const mat4* projection, const f64 delta) {
    // static f64 clock = CLOCK_TIME;
    // static bool flag = true;

    if (!edit) return;
    const frame_t* frame = ((comp_node_t*)edit->header.components)->root->component.data;
    const font_t* font = edit->font;
    const style_t* style = &edit->styles.normal;
    const vec4 border_color = {(f32)style->border.color.r / 255.0f, (f32)style->border.color.g / 255.0f, (f32)style->border.color.b / 255.0f, (f32)style->border.color.a / 255.0f};
    const vec2 dim = {(f32)edit->header.box.width, (f32)edit->header.box.height};

    if (edit->transform.init & 1) {
        const mat4 scale = m4_scale((f32)edit->header.box.width, (f32)edit->header.box.height, 1.0f);
        const mat4 position = m4_transl((f32)edit->header.box.x, (f32)edit->header.box.y, 0.0f);
        const mat4 size = m4_transl((f32)edit->header.box.width * 0.5f, (f32)edit->header.box.height * 0.5f, 0.0f);
        const mat4 inv_size = m4_transl(-(f32)edit->header.box.width * 0.5f, -(f32)edit->header.box.height * 0.5f, 0.0f);

        edit->transform.model = m4_mul(&position, &size);
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

    // if (frame->focused.data == button) color = (vec4){
    //     (f32)button->styles.hover.background.mask.r / 255.0f,
    //     (f32)button->styles.hover.background.mask.g / 255.0f,
    //     (f32)button->styles.hover.background.mask.b / 255.0f,
    //     (f32)button->styles.hover.background.mask.a / 255.0f
    // };
    // else color = (vec4){
    //     (f32)button->styles.normal.background.mask.r / 255.0f,
    //     (f32)button->styles.normal.background.mask.g / 255.0f,
    //     (f32)button->styles.normal.background.mask.b / 255.0f,
    //     (f32)button->styles.normal.background.mask.a / 255.0f
    // };
    set_vec4_uniform(edit->sprite->shader, "mask", ((vec4){1.0f, 1.0f, 1.0f, 1.0f}).e);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // todo: semi-working clock for the edit cursor
    // if (frame->focused.data == edit) {
    //     if (0.0 >= clock) {
    //         if (flag) {
    //             if (edit->font->mesh.count > 0)
    //                 edit->font->mesh.count--;
    //             flag = false;
    //         }
    //         else {
    //             edit->font->mesh.count++;
    //             flag = true;
    //         }
    //         clock = CLOCK_TIME;
    //     }
    //     if (clock > 0.0) {
    //         clock -= delta;
    //     }
    // }
    // else {
    //     if (edit->font->mesh.count > 0 && flag) {
    //         edit->font->mesh.count--;
    //         flag = false;
    //     }
    // }

    // draw the text mesh
    const mat4 position = m4_transl((f32)edit->header.box.x + style->border.thickness, (f32)edit->header.box.y + style->border.thickness, 0.0f);
    const mat4 size = m4_scale(1.0f, 1.0f, 1.0f);
    const mat4 model = m4_mul(&position, &size);
    bind_vertex_array(edit->mesh.va);
    glUseProgram(edit->mesh.shader->id);
    bind_font(edit->font);
    // ToDO: change this to work for a rotated edit
    glEnable(GL_SCISSOR_TEST);
    glScissor(
        edit->header.box.x , frame->header.box.height - edit->header.box.y - edit->header.box.height + style->border.thickness,
        edit->header.box.width - style->border.thickness, edit->header.box.height - style->border.thickness
    );
    set_mat4_uniform(edit->mesh.shader, "projection", true, projection->e);
    set_mat4_uniform(edit->mesh.shader, "model", true, model.e);
    set_vec4_uniform(edit->mesh.shader, "font.bg", color_v4(font->bg).e);
    set_vec4_uniform(edit->mesh.shader, "font.fg", color_v4(font->fg).e);
    glDrawArrays(GL_TRIANGLES, 0, 6 * (frame->focused.data == edit ? edit->mesh.count : edit->mesh.count - 1));
    glDisable(GL_SCISSOR_TEST);
}
