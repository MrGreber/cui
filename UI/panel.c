#include <panel.h>
#include <memio.h>
#include <component_system.h>
#include <frame.h>
#include <math-utils.h>
#include <shader/ops.h>
#include <geometry/ops.h>
#include <caption.h>
#include <events.h>

#include <memory.h>
#include <glad.h>
#include <glfw3.h>

static void private(mouse_callback)(const mouse_cb_param* param) {
    panel_t* panel = param->instance;

}
static void private(resize_callback)(const resize_cb_param* param) {
    panel_t* panel = param->instance;
    panel->header.dirty = 2;
    // comp_header_t* header = get_header(panel->parent);
    //
    // header->box.width += param->width;
    // header->box.height += param->height;
}

#define CAPTION_HEIGHT 30
panel_t* Panel(new)(void* parent, style_group_t* group, const bounding_box* box) {
    buf_t buffer = {
        .size = sizeof(panel_t),
        .tag = MEMTAG_PANEL
    };
    if (!Buffer(new)(&buffer, true)) return NULL;

    const comp_header_t* parent_header = get_header(parent);

    panel_t* panel = buffer.ptr;
    panel->header.dirty = 2;
    panel->header.box.x = box->x + parent_header->box.x;
    panel->header.box.y = box->y + parent_header->box.y;
    panel->header.box.width = box->width;
    panel->header.box.height = box->height;

    const i32 caption_height = (group->normal.modes & CAPTION) ? CAPTION_HEIGHT : 0;
    panel->header.content_box.x = box->x + parent_header->box.x;
    panel->header.content_box.y = box->y + parent_header->box.y + caption_height;
    panel->header.content_box.width = box->width;
    panel->header.content_box.height = box->height - caption_height;

    panel->header.parent = parent;
    if (group->normal.init) memcpy(&panel->styles.normal, &group->normal, sizeof(style_t));
    if (group->hover.init) memcpy(&panel->styles.hover, &group->hover, sizeof(style_t));

    frame_t* frame = get_root(parent);
    panel->sprite = Sprite(new)(frame, COMP_SHADER);
    if (!panel->sprite) goto cleanup;
    if (!Sprite(set_texture)(panel->sprite, box->width, box->height, &group->normal)) goto cleanup;

    //panel->header.mouse = (callback)private(mouse_callback);
    panel->header.update = (callback)Panel(update);
    panel->header.resize = (callback)private(resize_callback);
    panel->header.free = (callback)Panel(del);
    Component(push_node)(parent_header->components, panel, PANEL_COMPONENT);

    if (group->normal.modes & CAPTION) Caption(new)(panel);
    return panel;
cleanup:
    if (panel->sprite) Sprite(del)(panel->sprite);
    Buffer(del)(&(buf_t){.size = sizeof(panel_t), .tag = MEMTAG_PANEL, .ptr = panel});
    return NULL;
}
void Panel(del)(panel_t* panel) {
    if (!panel) return;
    if (panel->sprite) Sprite(del)(panel->sprite);
    Buffer(del)(&(buf_t){.size = sizeof(panel_t), .tag = MEMTAG_PANEL, .ptr = panel});
}
void Panel(bind)(const panel_t* panel) {
    if (!panel) return;
    Sprite(bind)(get_root(panel), panel->sprite);
}

void Panel(update)(panel_t* panel) {
    if (!panel || panel->header.hide || !panel->header.dirty) return;

    const frame_t* frame = get_root(panel);

    const mat4 scale = m4_scale((f32)panel->header.box.width, (f32)panel->header.box.height, 1.0f);
    const mat4 position = m4_transl((f32)panel->header.box.x, (f32)panel->header.box.y, 0.0f);
    const mat4 size = m4_transl((f32)panel->header.box.width * 0.5f, (f32)panel->header.box.height * 0.5f, 0.0f);
    const mat4 inv_size = m4_transl(-(f32)panel->header.box.width * 0.5f, -(f32)panel->header.box.height * 0.5f, 0.0f);

    panel->model = m4_mul(&position, &size);
    panel->model = m4_mul(&panel->model, &inv_size);
    panel->model = m4_mul(&panel->model, &scale);

    Sprite(bind)(frame, panel->sprite);
    Shader(set_mat4)(panel->sprite->shader, "projection", true, frame->cache.projection.e);
    Shader(set_mat4)(panel->sprite->shader, "model", true, panel->model.e);

    const style_t* style = &panel->styles.normal;
    vec4 color = Color(to_vec4)(style->border.color);
    const vec2 dim = {(f32)panel->header.box.width, (f32)panel->header.box.height};
    Shader(set_float)(panel->sprite->shader, "border.radius", style->border.radius);
    Shader(set_float)(panel->sprite->shader, "border.thickness", style->border.thickness);
    Shader(set_vec4)(panel->sprite->shader, "border.color", color.e);
    Shader(set_vec2)(panel->sprite->shader, "size", dim.e);
    color = Color(to_vec4)(panel->styles.normal.background.mask);
    Shader(set_vec4)(panel->sprite->shader, "mask", color.e);

    Mesh(draw)(panel->sprite->mesh);
    panel->header.dirty--;
}

void Panel(set_flag)(panel_t* panel, const u8 field) {
    if (!panel) return;

    switch (field) {
        case PANEL_HIDE: {
            panel->header.hide ^= 1;
            break;
        }
    }
}