#include <edit.h>
#include <memio.h>
#include <event_system.h>
#include <math-utils.h>
#include <frame.h>
#include <log.h>
#include <shader/ops.h>

#include <string.h>
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
static void build_text_mesh(edit_t* edit, f32 start_x, f32 start_y) {
    // TODO: optimize this function,
    // every AI I know of is dumb enough to not understand how to do it even though
    // the optimization is hella simple I mean I tried to do it myself for 2 times in a row
    // however it failed but I got close since I was able to render the text mesh semi correct and I know how to optimize
    // so i will try again sometime

    const str_t* buffer = edit->text.buffer;

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


    const f32 space_x = edit->font->glyphs[edit->font->amap(' ')].x_advance;
    for (u64 i = 0; i < buffer->length; i++) {
        const char c = buffer->data[i];

        if (c == '\n') {
            pen.x = start_x;
            pen.y += edit->font->line_height;
            if (edit->text.index == i + 1) {
                caret.pos = pen;
            }
            continue;
        }
        if (c == ' ') {
            pen.x += space_x;
            if (edit->text.index == i + 1) {
                caret.pos = pen;
            }
            continue;
        }
        if (c == '\t') {
            pen.x += 4.0f * space_x;
            if (edit->text.index == i + 1) {
                caret.pos = pen;
            }
            continue;
        }

        const u64 index = edit->font->amap(c);
        const glyph_t* g = &edit->font->glyphs[index];

        push_glyph_quad(edit, g, pen.x, pen.y);
        pen.x += g->x_advance;

        if (edit->text.index == i + 1) {
            caret.pos = pen;
        }
    }
    push_glyph_quad(edit, caret.glyph, caret.pos.x - (f32)caret.glyph->offset.x, caret.pos.y);

    VertexArray(bind)(edit->mesh.va);
    glUseProgram(edit->mesh.shader->id);
    bind_font(edit->font);
    glBindBuffer(GL_ARRAY_BUFFER, edit->mesh.vb->id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, edit->mesh.count * QUAD_SIZE, edit->mesh.vertices);
}
static void private(set_caret_position)(edit_t* edit, const f64 mouse_x, const f64 mouse_y) {
    const str_t* buffer = edit->text.buffer;

    glyph_t* caret_glyph = &edit->font->glyphs[edit->font->amap('|')];
    edit->mesh.count--;

    vec2 pen = { 0 };
    const f32 space_x = edit->font->glyphs[edit->font->amap(' ')].x_advance;

    for (u64 i = 0; i < buffer->length; i++) {
        const char c = buffer->data[i];
        const bool flag = (mouse_y >= pen.y && mouse_y < pen.y + edit->font->line_height);

        if (c == '\n') {
            if (!flag) {
                pen.x = 0.0f;
                pen.y += edit->font->line_height;
                edit->text.index = i + 1;
            }
            else break;
            continue;
        }

        f32 advance;
        switch (c) {
            case ' ': advance = space_x; break;
            case '\t': advance = 4.0f * space_x; break;
            default: advance = edit->font->glyphs[edit->font->amap(c)].x_advance; break;
        }
        if (!flag) continue;

        if (pen.x + advance < mouse_x) {
            pen.x += advance;
            edit->text.index = i + 1;
        }
        else break;
    }

    push_glyph_quad(edit, caret_glyph, pen.x - (f32)caret_glyph->offset.x, pen.y);

    VertexArray(bind)(edit->mesh.va);
    glUseProgram(edit->mesh.shader->id);
    bind_font(edit->font);
    glBindBuffer(GL_ARRAY_BUFFER, edit->mesh.vb->id);
    glBufferSubData(GL_ARRAY_BUFFER, (edit->mesh.count - 1) * QUAD_SIZE, QUAD_SIZE, edit->mesh.vertices + 6 * (edit->mesh.count - 1));
}

static void __default_mouse_callback(const mouse_cb_param* param) {
    edit_t* edit = param->instance;
    const frame_t* frame = get_root(edit);

    if (param->action == GLFW_PRESS) {
        private(set_caret_position)(edit, param->x, param->y);
    }
}
static void __default_write_keyboard_callback(const keyboard_cb_param* param) {
    edit_t* edit = param->instance;
    frame_t* frame = get_root(edit);

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
        build_text_mesh(edit, 0.0, 0.0);
    }
}
static void __default_read_keyboard_callback(const keyboard_cb_param* param) {
    edit_t* edit = param->instance;
    frame_t* frame = get_root(edit);

    bounding_box* box = &edit->header.box;
    if (param->action == GLFW_PRESS || param->action == GLFW_REPEAT) {
        switch (param->key) {
            case GLFW_KEY_LEFT_CONTROL:
            case GLFW_KEY_RIGHT_CONTROL:
            case GLFW_KEY_ESCAPE:
            case GLFW_KEY_LEFT_SHIFT:
            case GLFW_KEY_RIGHT_SHIFT: return;
            case GLFW_KEY_HOME: {
                edit->text.index = rfind_char(edit->text.buffer, edit->text.index, '\n');
                build_text_mesh(edit, 0.0, 0.0);
                break;
            }
            case GLFW_KEY_END: {
                edit->text.index = find_char(edit->text.buffer, edit->text.index, '\n');
                build_text_mesh(edit, 0.0, 0.0);
                break;
            }
            case GLFW_KEY_LEFT: {
                if (edit->text.index) {
                    edit->text.index--;
                    build_text_mesh(edit, 0.0, 0.0);
                }
                break;
            }
            case GLFW_KEY_RIGHT: {
                if (edit->text.index < edit->text.buffer->length) {
                    edit->text.index++;
                    build_text_mesh(edit, 0.0, 0.0);
                }
                break;
            }
            case GLFW_KEY_UP: {
                break;
            }
            case GLFW_KEY_DOWN: {
                break;
            }
            default: break;
        }
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
    build_text_mesh(edit, 0.0, 0.0);
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

    edit->header.box.x = box->x + parent_header->content_box.x;
    edit->header.box.y = box->y + parent_header->content_box.y;
    edit->header.box.width = box->width;
    edit->header.box.height = box->height;

    edit->parent = parent;
    if (group->normal.init) memcpy(&edit->styles.normal, &group->normal, sizeof(style_t));
    if (group->hover.init) memcpy(&edit->styles.hover, &group->hover, sizeof(style_t));

    frame_t* frame = get_root(parent);
    edit->sprite = new_sprite(frame, COMP_SHADER);
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

    edit->mesh.va = VertexArray(new)(2);
    edit->mesh.vb = VertexBuffer(new)(NULL, QUAD_SIZE * DEFAULT_CAPACITY, true);
    if (!edit->mesh.va || !edit->mesh.vb) goto cleanup;
    VertexArray(bind)(edit->mesh.va);
    VertexBuffer(bind)(edit->mesh.vb);

    VertexArray(push_f32)(edit->mesh.va, 2);
    VertexArray(push_f32)(edit->mesh.va, 2);
    VertexArray(push_buffer)(edit->mesh.va, edit->mesh.vb);

    edit->mesh.shader = Shader(get)(frame, TEXT_SHADER);
    if (!edit->mesh.shader) goto cleanup;

    edit->text.buffer = new_str("", 0);
    if (!edit->text.buffer) goto cleanup;

    edit->header.mouse = (callback)__default_mouse_callback;
    if (group->normal.mode) edit->header.keyboard = (callback)__default_write_keyboard_callback;
    else edit->header.keyboard = (callback)__default_read_keyboard_callback;
    edit->header.resize = (callback)__default_resize_callback;
    push_comp_node(parent_header->components, edit, EDIT_COMPONENT);

    build_text_mesh(edit, 0.0, 0.0);
    return edit;
cleanup:
    if (edit->mesh.va) VertexArray(del)(edit->mesh.va);
    if (edit->mesh.vb) VertexBuffer(del)(edit->mesh.vb);
    if (edit->mesh.vertices) del_buf(&(buf_t){.ptr = edit->mesh.vertices, .size = QUAD_SIZE * DEFAULT_CAPACITY, .tag = MEMTAG_VECTOR});
    if (edit->sprite) del_sprite(edit->sprite);
    if (edit->text.buffer) del_str(edit->text.buffer);
    if (edit->font) del_font(edit->font);
    del_buf(&(buf_t){.size = sizeof(edit_t), .tag = MEMTAG_EDIT, .ptr = edit});
    return NULL;
}
void del_edit(edit_t* edit) {
    if (!edit) return;
    VertexArray(del)(edit->mesh.va);
    VertexBuffer(del)(edit->mesh.vb);
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
void update_edit(edit_t* edit, const mat4* projection) {
    if (!edit) return;
    const frame_t* frame = get_root(edit);
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
    Shader(set_mat4)(edit->sprite->shader, "projection", true, projection->e);
    Shader(set_mat4)(edit->sprite->shader, "model", true, edit->transform.model.e);
    Shader(set_float)(edit->sprite->shader, "border.radius", style->border.radius);
    Shader(set_float)(edit->sprite->shader, "border.thickness", style->border.thickness);
    Shader(set_vec4)(edit->sprite->shader, "border.color", &border_color.x);
    Shader(set_vec2)(edit->sprite->shader, "size", dim.e);

    // if (frame->focused.data == edit) color = (vec4){
    //     (f32)edit->styles.hover.background.mask.r / 255.0f,
    //     (f32)edit->styles.hover.background.mask.g / 255.0f,
    //     (f32)edit->styles.hover.background.mask.b / 255.0f,
    //     (f32)edit->styles.hover.background.mask.a / 255.0f
    // };
    // else color = (vec4){
    //     (f32)edit->styles.normal.background.mask.r / 255.0f,
    //     (f32)edit->styles.normal.background.mask.g / 255.0f,
    //     (f32)edit->styles.normal.background.mask.b / 255.0f,
    //     (f32)edit->styles.normal.background.mask.a / 255.0f
    // };
    Shader(set_vec4)(edit->sprite->shader, "mask", &((vec4){.x = 1.0f, .y = 1.0f, .z = 1.0f, .w = 1.0f}).x);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // todo: semi-working clock for the edit cursor
    // static f64 clock = CLOCK_TIME;
    // static bool flag = true;
    // const f32 delta = (f32)frame->stopwatch.delta;
    // if (frame->focused.inst == edit) {
    //     if (0.0 >= clock) {
    //         if (flag) {
    //             if (edit->mesh.count > 0)
    //                 edit->mesh.count--;
    //             flag = false;
    //         }
    //         else {
    //             edit->mesh.count++;
    //             flag = true;
    //         }
    //         clock = CLOCK_TIME;
    //     }
    //     if (clock > 0.0) {
    //         clock -= delta;
    //     }
    // }
    // else {
    //     if (edit->mesh.count > 0 && flag) {
    //         edit->mesh.count--;
    //         flag = false;
    //     }
    // }

    // draw the text mesh
    const mat4 position = m4_transl((f32)edit->header.box.x + style->border.thickness, (f32)edit->header.box.y + style->border.thickness, 0.0f);
    const mat4 size = m4_scale(1.0f, 1.0f, 1.0f);
    const mat4 model = m4_mul(&position, &size);
    VertexArray(bind)(edit->mesh.va);
    glUseProgram(edit->mesh.shader->id);
    bind_font(edit->font);
    glEnable(GL_SCISSOR_TEST);
    glScissor(
        edit->header.box.x , frame->header.box.height - edit->header.box.y - edit->header.box.height + style->border.thickness,
        edit->header.box.width - style->border.thickness, edit->header.box.height - style->border.thickness
    );
    Shader(set_mat4)(edit->mesh.shader, "projection", true, projection->e);
    Shader(set_mat4)(edit->mesh.shader, "model", true, model.e);
    Shader(set_vec4)(edit->mesh.shader, "font.bg", color_v4(font->bg).e);
    Shader(set_vec4)(edit->mesh.shader, "font.fg", color_v4(font->fg).e);
    glDrawArrays(GL_TRIANGLES, 0, 6 * (frame->focused.inst == edit ? edit->mesh.count : edit->mesh.count - 1));
    glDisable(GL_SCISSOR_TEST);
}
