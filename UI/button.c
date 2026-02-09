#include <button.h>
#include <mem.h>
#include <event_system.h>
#include <frame.h>
#include <math-utils.h>

#include <memory.h>
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
    button->transform.init |= 1;
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
    button->transform.init = 1;
    button->header.box.x = box->x + parent_header->box.x;
    button->header.box.y = box->y + parent_header->box.y;
    button->header.box.width = box->width;
    button->header.box.height = box->height;
    button->parent = parent;
    if (group->normal.init) memcpy(&button->styles.normal, &group->normal, sizeof(style_t));
    if (group->hover.init) memcpy(&button->styles.hover, &group->hover, sizeof(style_t));

    button->sprite = new_sprite("__component__");
    if (!button->sprite) goto cleanup;
    if (!set_sprite_texture(button->sprite, box->width, box->height, &group->normal)) goto cleanup;

    button->header.mouse = (callback)__default_mouse_callback;
    button->header.resize =  (callback)__default_resize_callback;
    push_comp_node(parent_header->components, button, BUTTON_COMPONENT);
    return button;
cleanup:
    if (button->sprite) del_sprite(button->sprite);
    del_buf(&(buf_t){.size = sizeof(button_t), .tag = MEMTAG_BUTTON, .ptr = button});
    return NULL;
}
void del_button(button_t* button) {
    if (!button) return;
    if (button->sprite) del_sprite(button->sprite);
    del_buf(&(buf_t){.size = sizeof(button_t), .tag = MEMTAG_BUTTON, .ptr = button});
}
void bind_button(const button_t* button) {
    if (!button) return;
    bind_sprite(button->sprite);
}

void update_button(button_t* button, const mat4* projection) {
    if (!button) return;

    if (button->transform.init & 1) {
        const mat4 scale = m4_scale((f32)button->header.box.width, (f32)button->header.box.height, 1.0f);
        const mat4 position = m4_transl((f32)button->header.box.x, (f32)button->header.box.y, 0.0f);
        const mat4 size = m4_transl((f32)button->header.box.width * 0.5f, (f32)button->header.box.height * 0.5f, 0.0f);
        const mat4 inv_size = m4_transl(-(f32)button->header.box.width * 0.5f, -(f32)button->header.box.height * 0.5f, 0.0f);

        button->transform.model = m4_mul(&position, &size);
        button->transform.model = m4_mul(&button->transform.model, &inv_size);
        button->transform.model = m4_mul(&button->transform.model, &scale);
        button->transform.init ^= 1;
    }

    set_mat4_uniform(button->sprite->shader, "projection", true, projection->e);
    set_mat4_uniform(button->sprite->shader, "model", true, button->transform.model.e);

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
