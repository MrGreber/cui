#include <panel.h>
#include <mem.h>
#include <event_system.h>
#include <frame.h>
#include <math-utils.h>

#include <memory.h>
#include <glad.h>

static void __default_mouse_callback(const mouse_cb_param* param) {
    panel_t* panel = param->instance;
    frame_t* frame = ((comp_node_t*)panel->header.components)->root->component.data;
    printf("panel=%p\n", panel);
}
static void __default_resize_callback(const resize_cb_param* param) {
    panel_t* panel = param->instance;
    //comp_header_t* header = get_header(panel->parent);

    // panel->header.box.width += param->width;
    // panel->header.box.height += param->height;

}

panel_t* new_panel(void* parent, style_group_t* group, const bounding_box* box) {
    buf_t buffer = {
        .size = sizeof(panel_t),
        .tag = MEMTAG_PANEL
    };
    if (!new_buf(&buffer, true)) return NULL;

    const comp_header_t* parent_header = get_header(parent);

    panel_t* panel = buffer.ptr;
    panel->header.box.x = box->x + parent_header->box.x;
    panel->header.box.y = box->y + parent_header->box.y;
    panel->header.box.width = box->width;
    panel->header.box.height = box->height;
    panel->parent = parent;
    if (group->normal.init) memcpy(&panel->styles.normal, &group->normal, sizeof(style_t));
    if (group->hover.init) memcpy(&panel->styles.hover, &group->hover, sizeof(style_t));

    if (!gen_comp_texture(&panel->tex, box, &group->normal)) goto cleanup;

    // style_t* normal_style = NULL,* hover_style = NULL;
    // if (group->normal.init) normal_style = &group->normal;
    // if (group->hover.init) hover_style = &group->hover;

    panel->sprite = new_sprite("__component__");
    if (!panel->sprite) goto cleanup;

    panel->header.mouse = (callback)__default_mouse_callback;
    panel->header.resize = (callback)__default_resize_callback;

    push_comp_node(parent_header->components, panel, PANEL_COMPONENT);
    return panel;
cleanup:
    if (panel->sprite) del_sprite(panel->sprite);
    if (panel->tex) del_texture(panel->tex);
    del_buf(&(buf_t){.size = sizeof(panel_t), .tag = MEMTAG_PANEL, .ptr = panel});
    return NULL;
}
void del_panel(panel_t* panel) {
    if (!panel) return;
    if (panel->sprite) del_sprite(panel->sprite);
    if (panel->tex) del_texture(panel->tex);
    del_buf(&(buf_t){.size = sizeof(panel_t), .tag = MEMTAG_PANEL, .ptr = panel});
}
void bind_panel(const panel_t* panel) {
    if (!panel) return;
    bind_sprite(panel->sprite);
    bind_texture(panel->tex);
}

void update_panel(panel_t* panel, const mat4* projection, const f32 angle) {
    if (!panel) return;

    const mat4 rotation = m4_rotateZ(rad(angle));
    const mat4 scale = m4_scale((f32)panel->header.box.width, (f32)panel->header.box.height, 1.0f);
    const mat4 position = m4_transl((f32)panel->header.box.x, (f32)panel->header.box.y, 0.0f);
    const mat4 size = m4_transl((f32)panel->header.box.width * 0.5f, (f32)panel->header.box.height * 0.5f, 0.0f);
    const mat4 inv_size = m4_transl(-(f32)panel->header.box.width * 0.5f, -(f32)panel->header.box.height * 0.5f, 0.0f);

    mat4 model = m4_mul(&position, &size);
    model = m4_mul(&model, &rotation);
    model = m4_mul(&model, &inv_size);
    model = m4_mul(&model, &scale);
    set_mat4_uniform(panel->sprite->shader, "projection", true, projection->e);
    set_mat4_uniform(panel->sprite->shader, "model", true, model.e);

    const style_t* style = &panel->styles.normal;
    const color_t border_color = style->border.color;
    const vec4 color = {(f32)border_color.r / 255.0f, (f32)border_color.g / 255.0f, (f32)border_color.b / 255.0f, (f32)border_color.a / 255.0f};
    const vec2 dim = {(f32)panel->header.box.width, (f32)panel->header.box.height};
    set_float_uniform(panel->sprite->shader, "border.radius", style->border.radius);
    set_float_uniform(panel->sprite->shader, "border.thickness", style->border.thickness);
    set_vec4_uniform(panel->sprite->shader, "border.color", color.e);
    set_vec2_uniform(panel->sprite->shader, "size", dim.e);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}