#include <canvas.h>
#include <mem.h>
#include <event_system.h>
#include <frame.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glad.h>
#include <glfw3.h>
#include <glfw3native.h>

static void __default_mouse_callback(const mouse_cb_param* param) {
    canvas_t* canvas = param->instance;
    const frame_t* frame = ((comp_node_t*)canvas->header.components)->root->component.data;

    if (glfwGetMouseButton(frame->glfw_ctx, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (canvas->transform.init & 2) {
            const mat4 rotation = m4_rotateZ(rad(canvas->camera->roll));
            const mat4 scale = m4_scale(canvas->camera->zoom, canvas->camera->zoom, 1.0f);
            const mat4 position = m4_transl(canvas->camera->position.x, canvas->camera->position.y, 0.0f);
            const mat4 size = m4_transl((f32)canvas->dim.width * 0.5f, (f32)canvas->dim.height * 0.5f, 0.0f);
            const mat4 inv_size = m4_transl(-(f32)canvas->dim.width * 0.5f, -(f32)canvas->dim.height * 0.5f, 0.0f);
            canvas->transform.inv_model = m4_mul(&position, &size);
            canvas->transform.inv_model = m4_mul(&canvas->transform.inv_model, &rotation);
            canvas->transform.inv_model = m4_mul(&canvas->transform.inv_model, &inv_size);
            canvas->transform.inv_model = m4_mul(&canvas->transform.inv_model, &scale);
            canvas->transform.inv_model = m4_inverse(&canvas->transform.inv_model);
            canvas->transform.inv_model = m4_transp(&canvas->transform.inv_model);
            canvas->transform.init ^= 2;
        }
        vec4 mpos = {param->x, param->y, 0.0f, 1.0f};
        mpos = mv4_mul(&canvas->transform.inv_model, &mpos);

        if (canvas->prev.x != -1 && canvas->prev.y != -1) draw_texture_line(canvas->brush.color, canvas->prev.x, canvas->prev.y, mpos.x, mpos.y);
        else set_texture_pixel(canvas->brush.color, mpos.x, mpos.y);

        canvas->prev.x = mpos.x;
        canvas->prev.y = mpos.y;
    }
    else canvas->prev.x = canvas->prev.y = -1;
}
static void __default_keyboard_callback(const keyboard_cb_param* param) {
    canvas_t* canvas = param->instance;
    const frame_t* frame = ((comp_node_t*)canvas->header.components)->root->component.data;
    camera_t* camera = canvas->camera;

    if (param->action == GLFW_PRESS || param->action == GLFW_REPEAT) {
#ifndef SPEED
#define SPEED 10.0f
        switch (param->key) {
            case GLFW_KEY_Q: {
                camera->roll -= 2.0f;
                if (camera->roll < 0.0f) camera->roll += 360.0f;
                break;
            }
            case GLFW_KEY_E: {
                camera->roll += 2.0f;
                if (camera->roll > 360.0f) camera->roll -= 360.0f;
                break;
            }
            case GLFW_KEY_W: {
                camera->position.y -= SPEED;
                break;
            }
            case GLFW_KEY_S: {
                camera->position.y += SPEED;
                break;
            }
            case GLFW_KEY_A: {
                camera->position.x -= SPEED;
                break;
            }
            case GLFW_KEY_D: {
                camera->position.x += SPEED;
                break;
            }
            case GLFW_KEY_SPACE: {
                flush_texture(canvas->sprite->tex, WHITE);
                break;
            }
            case GLFW_KEY_R: {
                reset_camera(camera);
                break;
            }
            default: break;
        }

        canvas->transform.init |= 3;
#undef SPEED
#else
#error For some reason your dumbass decided to define a global macro named SPEED, what the fuck if you try to compiler me again I will send assassins after your ass
#endif
    }

}
static void __default_scroll_callback(const scroll_cb_param* param) {
    canvas_t* canvas = param->instance;
    const frame_t* frame = ((comp_node_t*)canvas->header.components)->root->component.data;
    camera_t* camera = canvas->camera;

    const f32 s = tanhf(param->delta);
    camera->zoom -= (f32)s;
    if (camera->zoom < 0.5f) camera->zoom = 0.5f;
    if (camera->zoom > 100.0f) camera->zoom = 100.0f;

    canvas->transform.init |= 3;
}
static void __default_resize_callback(const resize_cb_param* param) {
    canvas_t* canvas = param->instance;
    canvas->transform.init |= 3;
}


canvas_t* new_canvas(void* parent, const u32 width, const u32 height) {
    buf_t buffer = {
        .size = sizeof(canvas_t),
        .tag = MEMTAG_CANVAS
    };
    if (!new_buf(&buffer, true)) return NULL;

    const comp_header_t* parent_header = get_header(parent);

    canvas_t* canvas = buffer.ptr;
    canvas->transform.init = 3;
    canvas->dim.width = width;
    canvas->dim.height = height;
    canvas->header.box.x = parent_header->box.x;
    canvas->header.box.y = parent_header->box.y;
    canvas->header.box.width = parent_header->box.width;
    canvas->header.box.height = parent_header->box.height;
    canvas->parent = parent;
    canvas->prev.x = -1;
    canvas->prev.y = -1;

    canvas->sprite = new_sprite("__canvas__");
    if (!canvas->sprite) goto cleanup;
    if (!set_sprite_texture(canvas->sprite, width, height, &(style_t){.background = {.type = BG_COLOR, .color = WHITE}})) goto cleanup;

    canvas->camera = new_camera();
    if (!canvas->camera) goto cleanup;

    canvas->header.mouse = (callback)__default_mouse_callback;
    canvas->header.keyboard = (callback)__default_keyboard_callback;
    canvas->header.resize = (callback)__default_resize_callback;
    canvas->header.scroll = (callback)__default_scroll_callback;
    push_comp_node(parent_header->components, canvas, CANVAS_COMPONENT);
    return canvas;
cleanup:
    if (canvas->sprite) del_sprite(canvas->sprite);
    if (canvas->camera) del_camera(canvas->camera);
    del_buf(&(buf_t){.size = sizeof(canvas_t), .tag = MEMTAG_CANVAS, .ptr = canvas});
    return NULL;
}
void del_canvas(canvas_t* canvas) {
    if (!canvas) return;
    del_sprite(canvas->sprite);
    del_camera(canvas->camera);
    del_buf(&(buf_t){.size = sizeof(canvas_t), .tag = MEMTAG_CANVAS, .ptr = canvas});
}
void bind_canvas(canvas_t* canvas) {
    if (!canvas) return;
    bind_sprite(canvas->sprite);
}
void set_brush(canvas_t* canvas, const color_t color, const f32 size) {
    if (!canvas) return;
    canvas->brush.color = color;
    canvas->brush.size = size;
}

void update_canvas(canvas_t* canvas, const mat4* projection) {
    if (!canvas) return;
    const frame_t* frame = ((comp_node_t*)canvas->header.components)->root->component.data;
    if (canvas->transform.init & 1) {
        const mat4 rotation = m4_rotateZ(rad(canvas->camera->roll));
        const mat4 scale = m4_scale(canvas->camera->zoom * (f32)canvas->dim.width, canvas->camera->zoom * (f32)canvas->dim.height, 1.0f);
        const mat4 position = m4_transl(canvas->camera->position.x, canvas->camera->position.y, 0.0f);
        const mat4 size = m4_transl((f32)canvas->dim.width * 0.5f, (f32)canvas->dim.height * 0.5f, 0.0f);
        const mat4 inv_size = m4_transl(-(f32)canvas->dim.width * 0.5f, -(f32)canvas->dim.height * 0.5f, 0.0f);

        canvas->transform.model = m4_mul(&position, &size);
        canvas->transform.model = m4_mul(&canvas->transform.model, &rotation);
        canvas->transform.model = m4_mul(&canvas->transform.model, &inv_size);
        canvas->transform.model = m4_mul(&canvas->transform.model, &scale);
        canvas->transform.init ^= 1;
    }
    set_mat4_uniform(canvas->sprite->shader, "projection", true, projection->e);
    set_mat4_uniform(canvas->sprite->shader, "model", true, canvas->transform.model.e);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
}

