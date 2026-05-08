#include <caption.h>
#include <memio.h>
#include <component_system.h>
#include <frame.h>
#include <math-utils.h>
#include <shader/ops.h>
#include <geometry/ops.h>
#include <events.h>

#include <glad.h>
#include <glfw3.h>

static void private(mouse_callback)(const mouse_cb_param* param) {
    caption_t* caption = param->instance;
    frame_t* frame = get_root(caption);

    if (param->button == GLFW_MOUSE_BUTTON_LEFT && param->action == GLFW_PRESS) {
        caption->header.drag = true;
        caption->prev.x = param->x;
        caption->prev.y = param->y;

        frame->captured.instance = caption;
        frame->captured.tag = CAPTION_COMPONENT;
    }
    if (caption->header.drag) {
        const i32 dx = (i32)param->x - (i32)caption->prev.x;
        const i32 dy = (i32)param->y - (i32)caption->prev.y;

        caption->header.box.x += dx;
        caption->header.box.y += dy;
        caption->header.content_box.x += dx;
        caption->header.content_box.y += dy;

        caption->prev.x = param->x;
        caption->prev.y = param->y;

        caption->header.dirty = 1;

        comp_header_t* parent_header = get_header(caption->header.parent);
        Frame(clear)(frame, &parent_header->box);

        parent_header->box.x += dx;
        parent_header->box.y += dy;
        parent_header->content_box.x += dx;
        parent_header->content_box.y += dy;
        parent_header->dirty = 1;
    }
    if (param->button == GLFW_MOUSE_BUTTON_LEFT && param->action == GLFW_RELEASE) {
        caption->header.drag = false;
        frame->captured.instance = NULL;
        frame->captured.tag = 0;
    }
}
static void private(resize_callback)(const resize_cb_param* param) {
    caption_t* caption = param->instance;
    caption->header.dirty |= 1;
    // comp_header_t* header = get_header(caption->parent);
    // header->box.width += param->width;
    // header->box.height += param->height;
}

#define CAPTION_HEIGHT 30
caption_t* Caption(new)(void* parent) {
    buf_t buffer = {
        .size = sizeof(caption_t),
        .tag = MEMTAG_CAPTION
    };
    if (!Buffer(new)(&buffer, true)) return NULL;

    const comp_header_t* parent_header = get_header(parent);

    caption_t* caption = buffer.ptr;
    caption->header.dirty = 1;

    caption->header.box.x = parent_header->box.x;
    caption->header.box.y = parent_header->box.y;
    caption->header.box.width = parent_header->box.width;
    caption->header.box.height = CAPTION_HEIGHT;

    caption->header.content_box.x = parent_header->box.x;
    caption->header.content_box.y = parent_header->box.y;
    caption->header.content_box.width = parent_header->box.width;
    caption->header.content_box.height = CAPTION_HEIGHT;

    caption->header.parent = parent;
    frame_t* frame = get_root(parent);
    caption->sprite = Sprite(new)(frame, RECT_SHADER);
    if (!caption->sprite) goto cleanup;
    if (!Sprite(set_texture)(caption->sprite, parent_header->box.width, CAPTION_HEIGHT, &(style_t){.background = {.type = BG_COLOR, .color = RED}})) goto cleanup;

    caption->header.mouse = (callback)private(mouse_callback);
    caption->header.resize = (callback)private(resize_callback);
    caption->header.update = (callback)Caption(update);
    Component(push_node)(parent_header->components, caption, CAPTION_COMPONENT);

    return caption;
cleanup:
    if (caption->sprite) Sprite(del)(caption->sprite);
    Buffer(del)(&(buf_t){.size = sizeof(caption_t), .tag = MEMTAG_CAPTION, .ptr = caption});
    return NULL;

}
void Caption(del)(caption_t* caption) {
    if (!caption) return;
    if (caption->sprite) Sprite(del)(caption->sprite);
    Buffer(del)(&(buf_t){.size = sizeof(caption_t), .tag = MEMTAG_CAPTION, .ptr = caption});
}
void Caption(bind)(const caption_t* caption) {
    if (!caption) return;
    Sprite(bind)(get_root(caption), caption->sprite);
}
void Caption(update)(caption_t* caption) {
    if (!caption || !caption->header.dirty) return;

    const frame_t* frame = get_root(caption);

    const mat4 scale = m4_scale((f32)caption->header.box.width, (f32)caption->header.box.height, 1.0f);
    const mat4 position = m4_transl((f32)caption->header.box.x, (f32)caption->header.box.y, 0.0f);
    const mat4 size = m4_transl((f32)caption->header.box.width * 0.5f, (f32)caption->header.box.height * 0.5f, 0.0f);
    const mat4 inv_size = m4_transl(-(f32)caption->header.box.width * 0.5f, -(f32)caption->header.box.height * 0.5f, 0.0f);

    caption->model = m4_mul(&position, &size);
    caption->model = m4_mul(&caption->model, &inv_size);
    caption->model = m4_mul(&caption->model, &scale);

    Sprite(bind)(frame, caption->sprite);
    Shader(set_mat4)(caption->sprite->shader, "projection", true, frame->cache.projection.e);
    Shader(set_mat4)(caption->sprite->shader, "model", true, caption->model.e);
    Mesh(draw)(caption->sprite->mesh);

    caption->header.dirty = 0;

}