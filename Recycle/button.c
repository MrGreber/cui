#include <button.h>
#include <mem.h>
#include <frame.h>
#include <utils.h>

#include <glad.h>
#include <glfw3.h>
#include <stdio.h>

static void __default_mouse_movement_callback(const mouse_cb_param* param) {
    const button_t* btn = param->instance;
    const i32 mouse_x = (i32)param->x;
    const i32 mouse_y = (i32)param->y;
    const i32 button = param->button;
    const i32 action = param->action;

    if (bounded(mouse_x, mouse_y, btn->header.box.x, btn->header.box.y, btn->header.box.width, btn->header.box.height) && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        const frame_t* frame = btn->parent;
        glfwSetWindowShouldClose(frame->glfw_ctx, true);
        printf("(%d, %d)\n", mouse_x, mouse_y);
    }
}

button_t* new_button(void* parent, const color_t bg, const u32 x, const u32 y, const u32 width, const u32 height) {
    if (!parent || !width || !height) return NULL;

    buf_t buffer = {
        .size = sizeof(button_t),
        .tag = MEMTAG_BUTTON,
    };
    if (!new_buf(&buffer, true)) return NULL;

    button_t* btn = buffer.ptr;
    btn->header.box.width = width;
    btn->header.box.height = height;
    btn->header.box.x = x;
    btn->header.box.y = y;
    btn->bg = bg;
    btn->parent = parent;

    const frame_t* frame = (frame_t*)parent;

    btn->tex = new_texture(width, height);
    if (!btn->tex) goto cleanup;
    flush_texture(btn->tex, width, height, bg);

    btn->obj = new_sprite("__button__");
    if (!btn->obj) goto cleanup;

    btn->header.mouse = __default_mouse_movement_callback;
    return btn;
cleanup:
    if (btn->obj) del_sprite(btn->obj);
    if (btn->tex) del_texture(btn->tex);
    del_buf(&(buf_t){.size = sizeof(button_t), .tag = MEMTAG_BUTTON, .ptr = btn});
    return NULL;
}
void del_button(button_t* btn) {
    if (!btn) return;
    if (btn->obj) del_sprite(btn->obj);
    if (btn->tex) del_texture(btn->tex);
    del_buf(&(buf_t){.size = sizeof(button_t), .tag = MEMTAG_BUTTON, .ptr = btn});
}

void bind_button(const button_t* btn) {
    if (!btn) return;
    bind_sprite(btn->obj);
    bind_texture(btn->tex);
}
void update_button(const button_t* btn) {
    if (!btn) return;

    // draws the canvas
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}