#include <edit.h>
#include <memio.h>
#include <event_system.h>
#include <math-utils.h>
#include <frame.h>
#include <error.h>
#include <shader/ops.h>
#include <geometry/ops.h>

#include <string.h>
#include <glad.h>
#include <glfw3.h>

#define DEFAULT_CAPACITY 128
#define QUAD 6

static void private(push_glyph)(edit_t* edit, const glyph_t* g, const f32 pen_x, const f32 pen_y) {
    const f32 x0 = pen_x + g->offset.x;
    const f32 y0 = pen_y + g->offset.y;
    const f32 x1 = x0 + g->dim.width;
    const f32 y1 = y0 + g->dim.height;

    const f32 quad[24] = {
        x0, y1, g->x0, g->y1,
        x1, y0, g->x1, g->y0,
        x0, y0, g->x0, g->y0,
        x0, y1, g->x0, g->y1,
        x1, y1, g->x1, g->y1,
        x1, y0, g->x1, g->y0
    };
    Mesh(push)(edit->mesh, (mesh_buf_t){
        .data = quad,
        .count = QUAD,
        .is_vertices = true
    });
}
static void private(build_mesh)(edit_t* edit, const f32 start_x, const f32 start_y) {
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
    edit->mesh->vertices.count = 0;

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

        private(push_glyph)(edit, g, pen.x, pen.y);
        pen.x += g->x_advance;

        if (edit->text.index == i + 1) {
            caret.pos = pen;
        }
    }
    private(push_glyph)(edit, caret.glyph, caret.pos.x - (f32)caret.glyph->offset.x, caret.pos.y);

    Mesh(bind)(get_root(edit->parent), edit->mesh);
    Font(bind)(edit->font);
    Mesh(upload)(edit->mesh, edit->mesh->vertices.count, 0);
}
static void private(set_caret_position)(edit_t* edit, const f64 mouse_x, const f64 mouse_y) {
    const str_t* buffer = edit->text.buffer;

    const glyph_t* caret_glyph = &edit->font->glyphs[edit->font->amap('|')];
    if (edit->mesh->vertices.count >= QUAD) edit->mesh->vertices.count -= QUAD;

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
    private(push_glyph)(edit, caret_glyph, pen.x - (f32)caret_glyph->offset.x, pen.y);

    Mesh(bind)(get_root(edit->parent), edit->mesh);
    Font(bind)(edit->font);
    Mesh(upload)(edit->mesh, QUAD, edit->mesh->vertices.count - QUAD);
}

static void private(mouse_callback)(const mouse_cb_param* param) {
    edit_t* edit = param->instance;
    const frame_t* frame = get_root(edit);

    if (param->action == GLFW_PRESS) {
        private(set_caret_position)(edit, param->x, param->y);
    }
}
static void private(write_keyboard_callback)(const keyboard_cb_param* param) {
    edit_t* edit = param->instance;
    frame_t* frame = get_root(edit);

    bounding_box* box = &edit->header.box;
    if (param->action == GLFW_PRESS || param->action == GLFW_REPEAT) {
        switch (param->key) {
            case GLFW_KEY_ENTER: {
                String(insertC)(edit->text.buffer, edit->text.index++, _C_'\n');
                goto rebuild_text_mesh;
            }
            case GLFW_KEY_TAB: {
                String(insertC)(edit->text.buffer, edit->text.index++, _C_'\t');
                goto rebuild_text_mesh;
            }
            default: {
                const char_t key = edit->font->kmap(param->key, param->modes == GLFW_MOD_SHIFT);
                String(insertC)(edit->text.buffer, edit->text.index++, key);
                goto rebuild_text_mesh;
            }
            case GLFW_KEY_LEFT_CONTROL:
            case GLFW_KEY_RIGHT_CONTROL:
            case GLFW_KEY_ESCAPE:
            case GLFW_KEY_LEFT_SHIFT:
            case GLFW_KEY_RIGHT_SHIFT: return;
            case GLFW_KEY_HOME: {
                edit->text.index = String(rfindC)(edit->text.buffer, edit->text.index, '\n');
                goto rebuild_text_mesh;
            }
            case GLFW_KEY_END: {
                edit->text.index = String(findC)(edit->text.buffer, edit->text.index, '\n');
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
                    String(popC)(edit->text.buffer, --edit->text.index);
                    goto rebuild_text_mesh;
                }
                break;
            }
        }
    }
    return;

rebuild_text_mesh:
    if (edit->font) {
        private(build_mesh)(edit, 0.0, 0.0);
    }
}
static void private(read_keyboard_callback)(const keyboard_cb_param* param) {
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
                edit->text.index = String(rfindC)(edit->text.buffer, edit->text.index, '\n');
                private(build_mesh)(edit, 0.0, 0.0);
                break;
            }
            case GLFW_KEY_END: {
                edit->text.index = String(findC)(edit->text.buffer, edit->text.index, '\n');
                private(build_mesh)(edit, 0.0, 0.0);
                break;
            }
            case GLFW_KEY_LEFT: {
                if (edit->text.index) {
                    edit->text.index--;
                    private(build_mesh)(edit, 0.0, 0.0);
                }
                break;
            }
            case GLFW_KEY_RIGHT: {
                if (edit->text.index < edit->text.buffer->length) {
                    edit->text.index++;
                    private(build_mesh)(edit, 0.0, 0.0);
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
static void private(resize_callback)(const resize_cb_param* param) {
    edit_t* edit = param->instance;
    edit->transform.init |= 1;
    // comp_header_t* header = get_header(edit->parent);
    // edit->header.box.width += param->width;
    // edit->header.box.height += param->height;
}

void Edit(set_text)(edit_t* edit, char_t* text, const u64 length) {
    if (!String(set)(edit->text.buffer, text, length)) {
        logWarn(ERR_STRING, "Failed to set edit, text.");
        return;
    }
    private(build_mesh)(edit, 0.0, 0.0);
}

edit_t* Edit(new)(void* parent, const style_group_t* group, const bounding_box* box) {
    buf_t buffer = {
        .size = sizeof(edit_t),
        .tag = MEMTAG_EDIT
    };
    if (!Buffer(new)(&buffer, true)) return NULL;

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

    edit->sprite = Sprite(new)(frame, COMP_SHADER);
    if (!edit->sprite) goto cleanup;
    if (!Sprite(set_texture)(edit->sprite, box->width, box->height, (style_t*)&group->normal)) goto cleanup;

    // Loads default font
    edit->font = Font(new)(frame, __DIR__"\\Resources\\vcr_osd_mono.fnt");
    if (!edit->font) {
        logError(ERR_LOADING, "Failed to load font.");
        goto cleanup;
    }
    edit->mesh = Mesh(new)(frame, (mesh_param_t){
        .tag = DYNAMIC_MESH,
        .attributes = MESH_2D | MESH_UV0,
        .capacity = QUAD * DEFAULT_CAPACITY
    });
    if (!edit->mesh) {
        logError(ERR_COMPONENT_SYSTEM, "Failed to create edit text mesh.");
        goto cleanup;
    }

    edit->text.buffer = String(new)("", 0);
    if (!edit->text.buffer) goto cleanup;

    edit->header.mouse = (callback)private(mouse_callback);
    if (group->normal.mode) edit->header.keyboard = (callback)private(write_keyboard_callback);
    else edit->header.keyboard = (callback)private(read_keyboard_callback);
    edit->header.resize = (callback)private(resize_callback);
    Component(push_node)(parent_header->components, edit, EDIT_COMPONENT);

    private(build_mesh)(edit, 0.0, 0.0);
    return edit;
cleanup:
    if (edit->sprite) Sprite(del)(edit->sprite);
    if (edit->text.buffer) String(del)(edit->text.buffer);
    if (edit->font) Font(del)(edit->font);
    Buffer(del)(&(buf_t){.size = sizeof(edit_t), .tag = MEMTAG_EDIT, .ptr = edit});
    return NULL;
}
void Edit(del)(edit_t* edit) {
    if (!edit) return;
    Sprite(del)(edit->sprite);
    String(del)(edit->text.buffer);
    Font(del)(edit->font);
    Buffer(del)(&(buf_t){.size = sizeof(edit_t), .tag = MEMTAG_EDIT, .ptr = edit});
}
void Edit(bind)(const edit_t* edit) {
    if (!edit) return;
    const frame_t* frame = get_root(edit);
    Sprite(bind)(frame, edit->sprite);
}

#define CLOCK_TIME 0.02
void Edit(update)(edit_t* edit) {
    if (!edit) return;
    const frame_t* frame = get_root(edit);
    const font_t* font = edit->font;
    const style_t* style = &edit->styles.normal;
    const vec4 border_color = Color(to_vec4)(style->border.color);
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
    Sprite(bind)(frame, edit->sprite);
    Shader(set_mat4)(edit->sprite->shader, "projection", true, frame->cache.projection.e);
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
    glEnable(GL_SCISSOR_TEST);
    glScissor(
        edit->header.box.x , frame->header.box.height - edit->header.box.y - edit->header.box.height + style->border.thickness,
        edit->header.box.width - style->border.thickness, edit->header.box.height - style->border.thickness
    );
    Mesh(bind)(frame, edit->mesh);
    Font(bind)(edit->font);
    Shader(set_mat4)(edit->font->shader, "projection", true, frame->cache.projection.e);
    Shader(set_mat4)(edit->font->shader, "model", true, model.e);
    Shader(set_vec4)(edit->font->shader, "font.bg", Color(to_vec4)(font->bg).e);
    Shader(set_vec4)(edit->font->shader, "font.fg", Color(to_vec4)(font->fg).e);
    Mesh(sub_draw)(edit->mesh, (frame->focused.instance == edit ? edit->mesh->vertices.count : edit->mesh->vertices.count - QUAD), 0);
    glDisable(GL_SCISSOR_TEST);
}
