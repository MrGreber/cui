#include <button.h>
#include <memio.h>
#include <component_system.h>
#include <frame.h>
#include <math-utils.h>
#include <shader/ops.h>
#include <geometry/ops.h>
#include <events.h>

#include <memory.h>
#include <glad.h>
#include <glfw3.h>

static void __default_mouse_callback(const mouse_cb_param* param) {
    button_t* button = param->instance;
    frame_t* frame = get_root(button);

    if (param->action == GLFW_PRESS) {
        if (button->on_click) button->on_click(button);
        printf("button=%p\n", button);
        button->header.dirty = 2;
    }
}
static void __default_resize_callback(const resize_cb_param* param) {
    button_t* button = param->instance;
    button->header.dirty = 2;
    //comp_header_t* header = get_header(panel->parent);

    // panel->header.box.width += param->width;
    // panel->header.box.height += param->height;
}

button_t* Button(new)(void* parent, style_group_t* group, const bounding_box* box) {
    buf_t buffer = {
        .size = sizeof(button_t),
        .tag = MEMTAG_BUTTON
    };
    if (!Buffer(new)(&buffer, true)) return NULL;

    const comp_header_t* parent_header = get_header(parent);

    button_t* button = buffer.ptr;
    button->header.dirty = 2;
    button->header.box.x = box->x + parent_header->content_box.x;
    button->header.box.y = box->y + parent_header->content_box.y;
    button->header.box.width = box->width;
    button->header.box.height = box->height;
    button->header.parent = parent;
    if (group->normal.init) memcpy(&button->styles.normal, &group->normal, sizeof(style_t));
    if (group->hover.init) memcpy(&button->styles.hover, &group->hover, sizeof(style_t));

    frame_t* frame = get_root(parent);
    button->sprite = Sprite(new)(frame, COMP_SHADER);
    if (!button->sprite) goto cleanup;
    if (!Sprite(set_texture)(button->sprite, box->width, box->height, &group->normal)) goto cleanup;

    button->header.mouse = (callback)__default_mouse_callback;
    button->header.resize = (callback)__default_resize_callback;
    button->header.update = (callback)Button(update);
    button->header.free = (callback)Button(del);
    Component(push_node)(parent_header->components, button, BUTTON_COMPONENT);
    return button;
cleanup:
    if (button->sprite) Sprite(del)(button->sprite);
    Buffer(del)(&(buf_t){.size = sizeof(button_t), .tag = MEMTAG_BUTTON, .ptr = button});
    return NULL;
}
void Button(del)(button_t* button) {
    if (!button) return;
    if (button->sprite) Sprite(del)(button->sprite);
    Buffer(del)(&(buf_t){.size = sizeof(button_t), .tag = MEMTAG_BUTTON, .ptr = button});
}
void Button(bind)(const button_t* button) {
    if (!button) return;
    Sprite(bind)(get_root(button), button->sprite);
}

void Button(update)(button_t* button) {
    if (!button || !button->header.dirty) return;

    const frame_t* frame = get_root(button);

    const mat4 scale = m4_scale((f32)button->header.box.width, (f32)button->header.box.height, 1.0f);
    const mat4 position = m4_transl((f32)button->header.box.x, (f32)button->header.box.y, 0.0f);
    const mat4 size = m4_transl((f32)button->header.box.width * 0.5f, (f32)button->header.box.height * 0.5f, 0.0f);
    const mat4 inv_size = m4_transl(-(f32)button->header.box.width * 0.5f, -(f32)button->header.box.height * 0.5f, 0.0f);

    button->model = m4_mul(&position, &size);
    button->model = m4_mul(&button->model, &inv_size);
    button->model = m4_mul(&button->model, &scale);

    Sprite(bind)(frame, button->sprite);
    Shader(set_mat4)(button->sprite->shader, "projection", true, frame->cache.projection.e);
    Shader(set_mat4)(button->sprite->shader, "model", true, button->model.e);

    const style_t* style = &button->styles.normal;
    vec4 color = Color(to_vec4)(style->border.color);
    const vec2 dim = {(f32)button->header.box.width, (f32)button->header.box.height};
    Shader(set_float)(button->sprite->shader, "border.radius", style->border.radius);
    Shader(set_float)(button->sprite->shader, "border.thickness", style->border.thickness);
    Shader(set_vec4)(button->sprite->shader, "border.color", &color.x);
    Shader(set_vec2)(button->sprite->shader, "size", dim.e);
    if (frame->hovered.instance == button) color = Color(to_vec4)(button->styles.hover.background.mask);
    else color = Color(to_vec4)(button->styles.normal.background.mask);
    Shader(set_vec4)(button->sprite->shader, "mask", &color.x);

    Mesh(draw)(button->sprite->mesh);
    button->header.dirty--;
}