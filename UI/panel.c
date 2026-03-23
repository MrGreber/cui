#include <panel.h>
#include <memio.h>
#include <event_system.h>
#include <frame.h>
#include <math-utils.h>

#include <memory.h>
#include <glad.h>
#include <glfw3.h>

static void __default_mouse_callback(const mouse_cb_param* param) {
    panel_t* panel = param->instance;
    frame_t* frame = ((comp_node_t*)panel->header.components)->root->component.inst;

    if (param->action == GLFW_PRESS &&
        param->button == GLFW_MOUSE_BUTTON_LEFT &&
        (panel->styles.normal.mode & CAPTION) &&
        !(panel->styles.normal.mode & STATIC_POPUP))
    {
        panel->drag.state = true;
        panel->drag.prev.x = param->x;
        panel->drag.prev.y = param->y;

        frame->captured.inst = panel;
        frame->captured.tag  = PANEL_COMPONENT;
    }
    if (panel->drag.state) {
        const i32 dx = (i32)param->x - (i32)panel->drag.prev.x;
        const i32 dy = (i32)param->y - (i32)panel->drag.prev.y;

        panel->header.box.x += dx;
        panel->header.box.y += dy;
        panel->header.content_box.x += dx;
        panel->header.content_box.y += dy;

        panel->drag.prev.x = param->x;
        panel->drag.prev.y = param->y;

        panel->transform.init |= 1;
    }
    if (param->action == GLFW_RELEASE && param->button == GLFW_MOUSE_BUTTON_LEFT) {
        panel->drag.state = false;
        frame->captured.inst = NULL;
        frame->captured.tag  = 0;
    }
}
static void __default_resize_callback(const resize_cb_param* param) {
    panel_t* panel = param->instance;
    panel->transform.init |= 1;
    //comp_header_t* header = get_header(panel->parent);

    // panel->header.box.width += param->width;
    // panel->header.box.height += param->height;
}


#define CAPTION_HEIGHT 30
panel_t* new_panel(void* parent, style_group_t* group, const bounding_box* box) {
    buf_t buffer = {
        .size = sizeof(panel_t),
        .tag = MEMTAG_PANEL
    };
    if (!new_buf(&buffer, true)) return NULL;

    const comp_header_t* parent_header = get_header(parent);

    panel_t* panel = buffer.ptr;
    panel->transform.init = 1;
    panel->header.box.x = box->x + parent_header->box.x;
    panel->header.box.y = box->y + parent_header->box.y;
    panel->header.box.width = box->width;
    panel->header.box.height = box->height;

    const i32 caption_height = (group->normal.mode & CAPTION) ? CAPTION_HEIGHT : 0;
    panel->header.content_box.x = box->x + parent_header->box.x;
    panel->header.content_box.y = box->y + parent_header->box.y + caption_height;
    panel->header.content_box.width = box->width;
    panel->header.content_box.height = box->height - caption_height;

    panel->parent = parent;
    if (group->normal.init) memcpy(&panel->styles.normal, &group->normal, sizeof(style_t));
    if (group->hover.init) memcpy(&panel->styles.hover, &group->hover, sizeof(style_t));

    panel->sprite = new_sprite("__component__");
    if (!panel->sprite) goto cleanup;
    if (!set_sprite_texture(panel->sprite, box->width, box->height, &group->normal)) goto cleanup;

    panel->header.mouse = (callback)__default_mouse_callback;
    panel->header.resize = (callback)__default_resize_callback;
    push_comp_node(parent_header->components, panel, PANEL_COMPONENT);
    return panel;
cleanup:
    if (panel->sprite) del_sprite(panel->sprite);
    del_buf(&(buf_t){.size = sizeof(panel_t), .tag = MEMTAG_PANEL, .ptr = panel});
    return NULL;
}
void del_panel(panel_t* panel) {
    if (!panel) return;
    if (panel->sprite) del_sprite(panel->sprite);
    del_buf(&(buf_t){.size = sizeof(panel_t), .tag = MEMTAG_PANEL, .ptr = panel});
}
void bind_panel(const panel_t* panel) {
    if (!panel) return;
    bind_sprite(panel->sprite);
}

void update_panel(panel_t* panel, const mat4* projection) {
    if (!panel) return;

    if (panel->transform.init & 1) {
        const mat4 scale = m4_scale((f32)panel->header.box.width, (f32)panel->header.box.height, 1.0f);
        const mat4 position = m4_transl((f32)panel->header.box.x, (f32)panel->header.box.y, 0.0f);
        const mat4 size = m4_transl((f32)panel->header.box.width * 0.5f, (f32)panel->header.box.height * 0.5f, 0.0f);
        const mat4 inv_size = m4_transl(-(f32)panel->header.box.width * 0.5f, -(f32)panel->header.box.height * 0.5f, 0.0f);

        panel->transform.model = m4_mul(&position, &size);
        panel->transform.model = m4_mul(&panel->transform.model, &inv_size);
        panel->transform.model = m4_mul(&panel->transform.model, &scale);
        panel->transform.init ^= 1;
    }

    set_mat4_uniform(panel->sprite->shader, "projection", true, projection->e);
    set_mat4_uniform(panel->sprite->shader, "model", true, panel->transform.model.e);

    const style_t* style = &panel->styles.normal;
    const color_t border_color = style->border.color;
    const vec4 color = {(f32)border_color.r / 255.0f, (f32)border_color.g / 255.0f, (f32)border_color.b / 255.0f, (f32)border_color.a / 255.0f};
    const vec2 dim = {(f32)panel->header.box.width, (f32)panel->header.box.height};
    set_float_uniform(panel->sprite->shader, "border.radius", style->border.radius);
    set_float_uniform(panel->sprite->shader, "border.thickness", style->border.thickness);
    set_vec4_uniform(panel->sprite->shader, "border.color", color.e);
    set_vec2_uniform(panel->sprite->shader, "size", dim.e);
    set_vec4_uniform(panel->sprite->shader, "mask", ((vec4){.x = 1.0f, .y = 1.0f, .z = 1.0f, .w = 1.0f}).e);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}