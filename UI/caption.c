#include <caption.h>
#include <memio.h>
#include <event_system.h>
#include <frame.h>
#include <math-utils.h>
#include <shader/ops.h>
#include <geometry/ops.h>

static void private(mouse_callback)(const mouse_cb_param* param) {
    panel_t* panel = param->instance;
    frame_t* frame = get_root(panel);

    if (param->button == GLFW_MOUSE_BUTTON_LEFT && param->action == GLFW_PRESS &&
        (panel->styles.normal.mode & CAPTION) && !(panel->styles.normal.mode & STATIC_POPUP) &&
        !(bounded(param->x, param->y, panel->header.content_box.x, panel->header.content_box.y, panel->header.content_box.width, panel->header.content_box.height))
    ) {
        panel->drag.state = true;
        panel->drag.prev.x = param->x;
        panel->drag.prev.y = param->y;

        frame->captured.instance = panel;
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
    if (param->button == GLFW_MOUSE_BUTTON_LEFT && param->action == GLFW_RELEASE) {
        panel->drag.state = false;
        frame->captured.instance = NULL;
        frame->captured.tag  = 0;
    }
}
static void private(resize_callback)(const resize_cb_param* param) {
    panel_t* panel = param->instance;
    panel->transform.init |= 1;
    // comp_header_t* header = get_header(panel->parent);
    //
    // header->box.width += param->width;
    // header->box.height += param->height;
}

caption_t* Caption(new)(void* parent) {
    buf_t buffer = {
        .size = sizeof(caption_t),
        .tag = MEMTAG_CAPTION
    };
    if (!Buffer(new)(&buffer, true)) return NULL;

    const comp_header_t* parent_header = get_header(parent);

    caption_t* caption = buffer.ptr;
    caption->transform.init = 1;
    caption->header.box.x = box->x + parent_header->box.x;
    caption->header.box.y = box->y + parent_header->box.y;
    caption->header.box.width = box->width;
    caption->header.box.height = box->height;

    const i32 caption_height = (group->normal.mode & CAPTION) ? CAPTION_HEIGHT : 0;
    caption->header.content_box.x = box->x + parent_header->box.x;
    caption->header.content_box.y = box->y + parent_header->box.y + caption_height;
    caption->header.content_box.width = box->width;
    caption->header.content_box.height = box->height - caption_height;

    caption->parent = parent;
    frame_t* frame = get_root(parent);
    caption->sprite = Sprite(new)(frame, COMP_SHADER);
    if (!caption->sprite) goto cleanup;
    if (!Sprite(set_texture)(caption->sprite, box->width, box->height, &group->normal)) goto cleanup;

    caption->header.mouse = (callback)private(mouse_callback);
    caption->header.resize = (callback)private(resize_callback);
    Component(push_node)(parent_header->components, caption, PANEL_COMPONENT);
    return caption;
    cleanup:
        if (caption->sprite) Sprite(del)(caption->sprite);
    Buffer(del)(&(buf_t){.size = sizeof(panel_t), .tag = MEMTAG_PANEL, .ptr = caption});
    return NULL;

}
void Caption(del)(caption_t* caption) {

}
void Caption(bind)(const caption_t* caption) {

}
void Caption(update)(caption_t* caption) {

}