#include <canvas.h>
#include <mem.h>
#include <frame.h>
#include <event_system.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glad.h>
#include <glfw3.h>
#include <glfw3native.h>
#include <math.h>
#include <stdio.h>


extern bool __check_press(const frame_t*, const u32);
static void __default_mouse_movement_callback(const mouse_cb_param* param) {
    static int prev_px = -1, prev_py = -1;
    canvas_t* can = param->instance;
    const camera_t* cam = can->camera;
    frame_t* frame = can->parent;

    if (glfwGetMouseButton(frame->glfw_ctx, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        frame->focused.data = can;
        frame->focused.tag = CANVAS_COMPONENT;

        const color_t color = can->brush.type == BR_ERS ? TRANSP : can->brush.color;

        const f64 mouse_x = param->x;
        const f64 mouse_y = param->y;

        const f32 inv_frame_width = 1.0f / frame->header.box.width;
        const f32 inv_frame_height = 1.0f / frame->header.box.height;
        const f32 inv_zoom = 1.0f / cam->zoom;

        const f32 mouse_ndc_x = 2.0f * (f32)mouse_x * inv_frame_width - 1.0f;
        const f32 mouse_ndc_y = 1.0f - 2.0f * (f32)mouse_y * inv_frame_height;

        const f32 ndc_x0 = 2.0f * (f32)can->header.box.x * inv_frame_width - 1.0f;
        const f32 ndc_x1 = 2.0f * (f32)(can->header.box.x + can->header.box.width) * inv_frame_width - 1.0f;
        const f32 ndc_y0 = 1.0f - 2.0f * (f32)(can->header.box.y + can->header.box.height) * inv_frame_height;
        const f32 ndc_y1 = 1.0f - 2.0f * (f32)can->header.box.y * inv_frame_height;

        const f32 quad_center_x = (ndc_x0 + ndc_x1) * 0.5f;
        const f32 quad_center_y = (ndc_y0 + ndc_y1) * 0.5f;

        bind_canvas(can);
        set_vec2_uniform(can->obj->shader, "canvas_center", ((f32[2]){quad_center_x, quad_center_y}));

        f32 rx = mouse_ndc_x - quad_center_x;
        f32 ry = mouse_ndc_y - quad_center_y;

        rx += cam->position.x;
        ry += cam->position.y;

        rx *= inv_zoom;
        ry *= inv_zoom;

        rotZvp(&rx, &ry, -rad(cam->roll));

        const f32 canvas_ndc_x = rx + quad_center_x;
        const f32 canvas_ndc_y = ry + quad_center_y;

        if (canvas_ndc_x < ndc_x0 || canvas_ndc_x > ndc_x1 || canvas_ndc_y < ndc_y0 || canvas_ndc_y > ndc_y1) return;

        const f32 pct_x = (canvas_ndc_x - ndc_x0) / (ndc_x1 - ndc_x0);
        const f32 pct_y = (canvas_ndc_y - ndc_y0) / (ndc_y1 - ndc_y0);

        const i32 px = (i32)(pct_x * can->header.box.width);
        const i32 py = (i32)(pct_y * can->header.box.height);

        if (px < 0 || py < 0 || px >= can->header.box.width || py >= can->header.box.height) return;

        if (prev_px != -1 && prev_py != -1) draw_line(color, prev_px, prev_py, px, py);
        else draw_pixel(color, px, py);

        prev_px = px;
        prev_py = py;
    }
    else prev_px = prev_py = -1;
}
static void __default_keyboard_callback(const keyboard_cb_param* param) {
    const canvas_t* can = param->instance;
    const frame_t* frame = can->parent;
    camera_t* cam = can->camera;

    if (__check_press(frame, GLFW_KEY_SPACE)) flush_canvas(can);
    if (__check_press(frame, GLFW_KEY_LEFT_ALT)) {
        if (can->brush.type == BR_PX1) set_canvas_brush((canvas_t*)can, TRANSP, BR_ERS);
        else if (can->brush.type == BR_ERS) set_canvas_brush((canvas_t*)can, BLACK, BR_PX1);
    }
    if (glfwGetKey(frame->glfw_ctx, GLFW_KEY_W) == GLFW_PRESS) cam->position.y += 0.02f;
    if (glfwGetKey(frame->glfw_ctx, GLFW_KEY_S) == GLFW_PRESS) cam->position.y -= 0.02f;
    if (glfwGetKey(frame->glfw_ctx, GLFW_KEY_A) == GLFW_PRESS) cam->position.x += 0.02f;
    if (glfwGetKey(frame->glfw_ctx, GLFW_KEY_D) == GLFW_PRESS) cam->position.x -= 0.02f;

    if (glfwGetKey(frame->glfw_ctx, GLFW_KEY_Q) == GLFW_PRESS) cam->roll -= 0.1f;
    if (glfwGetKey(frame->glfw_ctx, GLFW_KEY_E) == GLFW_PRESS) cam->roll += 0.1f;

    if (glfwGetKey(frame->glfw_ctx, GLFW_KEY_R) == GLFW_PRESS) reset_camera(cam);

    if (cam->roll > deg(PI2)) cam->roll -= deg(PI2);
    if (cam->roll < 0.0f) cam->roll += deg(PI2);
}
static void __default_scroll_callback(const scroll_cb_param* param) {
    const canvas_t* can = param->instance;
    camera_t* cam = can->camera;
    const frame_t* frame = can->parent;
    const f32 scroll_y = (f32)param->delta;

    f64 mouse_x = 0.0, mouse_y = 0.0;
    glfwGetCursorPos(frame->glfw_ctx, &mouse_x, &mouse_y);

    if (mouse_x < can->header.box.x || mouse_x >= can->header.box.x + can->header.box.width || mouse_y < can->header.box.y || mouse_y >= can->header.box.y + can->header.box.height) return;

    const f32 inv_frame_width = 1.0f / frame->header.box.width;
    const f32 inv_frame_height = 1.0f / frame->header.box.height;
    const f32 inv_zoom = 1.0f / cam->zoom;

    const f32 ndc_x0 = 2.0f * (f32)can->header.box.x * inv_frame_width - 1.0f;
    const f32 ndc_x1 = 2.0f * (f32)(can->header.box.x + can->header.box.width) * inv_frame_width - 1.0f;
    const f32 ndc_y0 = 1.0f - 2.0f * (f32)(can->header.box.y + can->header.box.height) * inv_frame_height;
    const f32 ndc_y1 = 1.0f - 2.0f * (f32)can->header.box.y * inv_frame_height;

    const f32 quad_center_x = (ndc_x0 + ndc_x1) * 0.5f;
    const f32 quad_center_y = (ndc_y0 + ndc_y1) * 0.5f;

    const f32 mouse_ndc_x = 2.0f * (f32)mouse_x * inv_frame_width - 1.0f;
    const f32 mouse_ndc_y = 1.0f - 2.0f * (f32)mouse_y * inv_frame_height;

    f32 rx = mouse_ndc_x - quad_center_x;
    f32 ry = mouse_ndc_y - quad_center_y;

    rx += cam->position.x;
    ry += cam->position.y;

    rx *= inv_zoom;
    ry *= inv_zoom;

    rotZvp(&rx, &ry, -rad(cam->roll));

    const f32 logical_x = rx + quad_center_x;
    const f32 logical_y = ry + quad_center_y;

    const f32 s = tanhf(scroll_y);
    f32 new_zoom = cam->zoom - (f32)s;
    if (new_zoom < 0.5f) new_zoom = 0.5f;
    if (new_zoom > 100.0f) new_zoom = 100.0f;

    cam->zoom = new_zoom;

    f32 rx_new = logical_x - quad_center_x;
    f32 ry_new = logical_y - quad_center_y;

    rotZvp(&rx_new, &ry_new, rad(cam->roll));

    rx_new *= cam->zoom;
    ry_new *= cam->zoom;

    cam->position.x = rx_new - (mouse_ndc_x - quad_center_x);
    cam->position.y = ry_new - (mouse_ndc_y - quad_center_y);
}
static void __default_resize_callback(const resize_cb_param* param) {
    canvas_t* can = param->instance;

    resize_canvas(
        can,
        can->header.box.x, can->header.box.y,
        can->header.box.width, can->header.box.height,
        param->width, param->height
    );
}

canvas_t* new_canvas(void* parent, const color_t bg, const u32 x, const u32 y, const u32 width, const u32 height) {
    buf_t buffer = {
        .size = sizeof(canvas_t),
        .tag = MEMTAG_CANVAS,
    };
    if (!new_buf(&buffer, true)) return NULL;

    canvas_t* can = buffer.ptr;
    can->header.box.width = width;
    can->header.box.height = height;
    can->header.box.x = x;
    can->header.box.y = y;
    can->bg = bg;
    can->parent = parent;

    const frame_t* frame = (frame_t*)parent;

    can->tex = new_texture(width, height);
    if (!can->tex) goto cleanup;
    flush_texture(can->tex, can->header.box.width, can->header.box.height, can->bg);

    can->obj = new_sprite("__canvas__");
    if (!can->obj) goto cleanup;

    can->camera = new_camera();
    if (!can->camera) goto cleanup;

    can->header.mouse = __default_mouse_movement_callback;
    can->header.keyboard = __default_keyboard_callback;
    can->header.scroll = __default_scroll_callback;
    can->header.resize = __default_resize_callback;
    return can;
cleanup:
    if (can->obj) del_sprite(can->obj);
    if (can->tex) del_texture(can->tex);
    if (can->camera) del_camera(can->camera);
    del_buf(&(buf_t){.size = sizeof(canvas_t), .tag = MEMTAG_CANVAS, .ptr = can});
    return NULL;
}
void del_canvas(canvas_t* can) {
    if (!can) return;
    if (can->obj) del_sprite(can->obj);
    if (can->tex) del_texture(can->tex);
    if (can->camera) del_camera(can->camera);
    del_buf(&(buf_t){.size = sizeof(canvas_t), .tag = MEMTAG_CANVAS, .ptr = can});
}

void bind_canvas(const canvas_t* can) {
    if (!can) return;
    bind_sprite(can->obj);
    bind_texture(can->tex);
}
void update_canvas(const canvas_t* can) {
    if (!can) return;

    const frame_t* frame = ((comp_node_t*)can->header.components)->root->component.data;

    glEnable(GL_SCISSOR_TEST);
    glScissor(
        can->header.box.x, frame->header.box.height - can->header.box.y - can->header.box.height,
        can->header.box.width, can->header.box.height
    );
    // draws the canvas
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glDisable(GL_SCISSOR_TEST);
}
void flush_canvas(const canvas_t* can) {
    flush_texture(can->tex, can->header.box.width, can->header.box.height, can->bg);
}
bool resize_canvas(canvas_t* can, const u32 x, const u32 y, const u32 width, const u32 height, const u32 frame_width, const u32 frame_height) {
    if (!can) return false;

    sprite_t* new_obj = new_sprite("__canvas__");
    if (!new_obj) return false;

    sprite_t* old_obj = can->obj;
    can->obj = new_obj;
    can->header.box.width = width;
    can->header.box.height = height;
    can->header.box.x = x;
    can->header.box.y = y;

    if (old_obj) del_sprite(old_obj);
    bind_sprite(can->obj);

    return true;
}

void draw_pixel(const color_t color, const i32 x, const i32 y) {
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color);
}
void draw_line(const color_t color, i32 x0, i32 y0, const i32 x1, const i32 y1) {
    const i32 dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    const i32 dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    i32 err = dx + dy;

    for (;;) {
        draw_pixel(color, x0, y0);
        if (x0 == x1 && y0 == y1) break;
        const i32 e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
void set_canvas_brush(canvas_t* can, const color_t color, const brush_type type) {
    if (type == BR_PX1) can->brush.color = color;
    can->brush.type = type;
    can->brush.size = 1.0f;
}
