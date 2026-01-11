#include <button.h>
#include <mem.h>
#include <event_system.h>
#include <frame.h>
#include <math-utils.h>

#include <corecrt_memcpy_s.h>
#include <glad.h>
#include <glfw3.h>

static void __default_mouse_callback(const mouse_cb_param* param) {
    button_t* button = param->instance;
    frame_t* frame = ((comp_node_t*)button->header.components)->root->component.data;

    if (param->action == GLFW_PRESS) {
        if (button->on_click) button->on_click(button);
        printf("button=%p\n", button);
    }
}
static void __default_resize_callback(const resize_cb_param* param) {
    button_t* button = param->instance;
    //comp_header_t* header = get_header(panel->parent);

    // panel->header.box.width += param->width;
    // panel->header.box.height += param->height;

}

button_t* new_button(void* parent, style_group_t* group, const bounding_box* box) {
    buf_t buffer = {
        .size = sizeof(button_t),
        .tag = MEMTAG_BUTTON
    };
    if (!new_buf(&buffer, true)) return NULL;

    const comp_header_t* parent_header = get_header(parent);

    button_t* button = buffer.ptr;
    button->header.box.x = box->x + parent_header->box.x;
    button->header.box.y = box->y + parent_header->box.y;
    button->header.box.width = box->width;
    button->header.box.height = box->height;
    button->parent = parent;
    if (group->normal.init) memcpy_s(&button->styles.normal, sizeof(style_t), &group->normal, sizeof(style_t));
    if (group->hover.init) memcpy_s(&button->styles.hover, sizeof(style_t), &group->hover, sizeof(style_t));

    if (!gen_comp_texture(&button->tex, box, &group->normal)) goto cleanup;

    // style_t* normal_style = NULL,* hover_style = NULL;
    // if (group->normal.init) normal_style = &group->normal;
    // if (group->hover.init) hover_style = &group->hover;

    button->sprite = new_sprite("__component__");
    if (!button->sprite) goto cleanup;

    button->header.mouse = __default_mouse_callback;
    button->header.resize =  __default_resize_callback;

    push_comp_node(parent_header->components, button, PANEL_COMPONENT);
    return button;
cleanup:
    if (button->sprite) del_sprite(button->sprite);
    if (button->tex) del_texture(button->tex);
    del_buf(&(buf_t){.size = sizeof(button_t), .tag = MEMTAG_BUTTON, .ptr = button});
    return NULL;
}
void del_button(button_t* button) {
    if (!button) return;
    if (button->sprite) del_sprite(button->sprite);
    if (button->tex) del_texture(button->tex);
    del_buf(&(buf_t){.size = sizeof(button_t), .tag = MEMTAG_BUTTON, .ptr = button});
}
void bind_button(const button_t* button) {
    if (!button) return;
    bind_sprite(button->sprite);
    bind_texture(button->tex);
}

void update_button(button_t* button, const mat4* projection, const f32 angle) {
    if (!button) return;

    const mat4 rotation = m4_rotateZ(rad(angle));
    const mat4 scale = m4_scale((f32)button->header.box.width, (f32)button->header.box.height, 1.0f);
    const mat4 position = m4_transl((f32)button->header.box.x, (f32)button->header.box.y, 0.0f);
    const mat4 size = m4_transl((f32)button->header.box.width * 0.5f, (f32)button->header.box.height * 0.5f, 0.0f);
    const mat4 inv_size = m4_transl(-(f32)button->header.box.width * 0.5f, -(f32)button->header.box.height * 0.5f, 0.0f);

    mat4 model = m4_mul(&position, &size);
    model = m4_mul(&model, &rotation);
    model = m4_mul(&model, &inv_size);
    model = m4_mul(&model, &scale);
    set_mat4_uniform(button->sprite->shader, "projection", true, projection->e);
    set_mat4_uniform(button->sprite->shader, "model", true, model.e);

    const style_t* style = &button->styles.normal;
    const color_t border_color = style->border.color;
    const vec4 color = {(f32)border_color.r / 255.0f, (f32)border_color.g / 255.0f, (f32)border_color.b / 255.0f, (f32)border_color.a / 255.0f};
    const vec2 dim = {(f32)button->header.box.width, (f32)button->header.box.height};
    set_float_uniform(button->sprite->shader, "border.radius", style->border.radius);
    set_float_uniform(button->sprite->shader, "border.thickness", style->border.thickness);
    set_vec4_uniform(button->sprite->shader, "border.color", color.e);
    set_vec2_uniform(button->sprite->shader, "size", dim.e);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
