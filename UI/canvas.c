#include <canvas.h>
#include <mem.h>
#include <event_system.h>
#include <frame.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <memory.h>
#include <glad.h>
#include <glfw3.h>
#include <glfw3native.h>

static void __default_mouse_callback(const mouse_cb_param* param) {
    canvas_t* canvas = param->instance;
    const frame_t* frame = ((comp_node_t*)canvas->header.components)->root->component.data;

    if (glfwGetMouseButton(frame->glfw_ctx, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {

        if (canvas->prev.x != -1 && canvas->prev.y != -1) draw_texture_line(BLACK, canvas->prev.x, canvas->prev.y, param->x, param->y);
        else set_texture_pixel(BLACK, param->x, param->y);

        canvas->prev.x = param->x;
        canvas->prev.y = param->y;
    }

}
static void __default_keyboard_callback(const keyboard_cb_param* param) {
    const canvas_t* canvas = param->instance;
    const frame_t* frame = ((comp_node_t*)canvas->header.components)->root->component.data;
}
static void __default_resize_callback(const resize_cb_param* param) {
    const canvas_t* canvas = param->instance;
    // comp_header_t* header = get_header(edit->parent);
    // edit->header.box.width += param->width;
    // edit->header.box.height += param->height;
}


canvas_t* new_canvas(void* parent, const bounding_box* box) {
    buf_t buffer = {
        .size = sizeof(canvas_t),
        .tag = MEMTAG_CANVAS
    };
    if (!new_buf(&buffer, true)) return NULL;

    const comp_header_t* parent_header = get_header(parent);

    canvas_t* canvas = buffer.ptr;
    canvas->header.box.x = box->x + parent_header->box.x;
    canvas->header.box.y = box->y + parent_header->box.y;
    canvas->header.box.width = box->width;
    canvas->header.box.height = box->height;
    canvas->parent = parent;
    canvas->prev.x = -1;
    canvas->prev.y = -1;
    if (!gen_comp_texture(&canvas->tex, box, &(style_t){.background = {.type = BG_COLOR, .color = WHITE}})) goto cleanup;

    canvas->sprite = new_sprite("__canvas__");
    if (!canvas->sprite) goto cleanup;

    canvas->camera = new_camera();
    if (!canvas->camera) goto cleanup;

    canvas->header.mouse = (callback)__default_mouse_callback;
    canvas->header.keyboard = (callback)__default_keyboard_callback;
    canvas->header.resize = (callback)__default_resize_callback;
    push_comp_node(parent_header->components, canvas, CANVAS_COMPONENT);
    return canvas;
cleanup:
    if (canvas->sprite) del_sprite(canvas->sprite);
    if (canvas->tex) del_texture(canvas->tex);
    if (canvas->camera) del_camera(canvas->camera);
    del_buf(&(buf_t){.size = sizeof(canvas_t), .tag = MEMTAG_CANVAS, .ptr = canvas});
    return NULL;
}
void del_canvas(canvas_t* canvas) {
    if (!canvas) return;
    del_sprite(canvas->sprite);
    del_texture(canvas->tex);
    del_camera(canvas->camera);
    del_buf(&(buf_t){.size = sizeof(canvas_t), .tag = MEMTAG_CANVAS, .ptr = canvas});
}
void bind_canvas(canvas_t* canvas) {
    if (!canvas) return;
    bind_sprite(canvas->sprite);
    bind_texture(canvas->tex);
}
void set_brush(canvas_t* canvas, const color_t color, const f32 size) {
    if (!canvas) return;
    canvas->brush.color = color;
    canvas->brush.size = size;
}

void update_canvas(canvas_t* canvas, const mat4* projection) {
    if (!canvas) return;
    const frame_t* frame = ((comp_node_t*)canvas->header.components)->root->component.data;

    const mat4 rotation = m4_rotateZ(rad(0.0f));
    const mat4 scale = m4_scale((f32)canvas->header.box.width, (f32)canvas->header.box.height, 1.0f);
    const mat4 position = m4_transl((f32)canvas->header.box.x, (f32)canvas->header.box.y, 0.0f);
    const mat4 size = m4_transl((f32)canvas->header.box.width * 0.5f, (f32)canvas->header.box.height * 0.5f, 0.0f);
    const mat4 inv_size = m4_transl(-(f32)canvas->header.box.width * 0.5f, -(f32)canvas->header.box.height * 0.5f, 0.0f);

    mat4 model = m4_mul(&position, &size);
    model = m4_mul(&model, &rotation);
    model = m4_mul(&model, &inv_size);
    model = m4_mul(&model, &scale);
    set_mat4_uniform(canvas->sprite->shader, "projection", true, projection->e);
    set_mat4_uniform(canvas->sprite->shader, "model", true, model.e);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

}

