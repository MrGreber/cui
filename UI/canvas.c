#include <canvas.h>
#include <memio.h>
#include <component_system.h>
#include <frame.h>
#include <shader/ops.h>
#include <geometry/ops.h>
#include <events.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glad.h>
#include <glfw3.h>
#include <glfw3native.h>


#define CAM_KEY_W (1 << 0)
#define CAM_KEY_S (1 << 1)
#define CAM_KEY_A (1 << 2)
#define CAM_KEY_D (1 << 3)
#define CAM_KEY_Q (1 << 4)
#define CAM_KEY_E (1 << 5)
#define CAM_KEY_R (1 << 6)

static void private(camera_handler)(canvas_t* canvas) {
    const frame_t* frame = get_root(canvas);
    const f32 delta = (f32)frame->stopwatch.delta;
    camera_t* camera = &canvas->camera;
    comp_header_t* parent_header = get_header(canvas->header.parent);
#ifndef SPEED
#define SPEED 200.0f
    if (!camera->keys) return;
    if (camera->keys & CAM_KEY_W) camera->position.y -= SPEED * delta;
    if (camera->keys & CAM_KEY_S) camera->position.y += SPEED * delta;
    if (camera->keys & CAM_KEY_A) camera->position.x -= SPEED * delta;
    if (camera->keys & CAM_KEY_D) camera->position.x += SPEED * delta;
    if (camera->keys & CAM_KEY_Q) {
        camera->roll -= Camera(to_angle16)(SPEED * delta);
    }
    if (camera->keys & CAM_KEY_E) {
        camera->roll += Camera(to_angle16)(SPEED * delta);
    }
    if (camera->keys & CAM_KEY_R) Camera(reset)(camera);
    parent_header->dirty = 2;
#undef SPEED
#else
#error For some reason your dumbass decided to define a global macro named SPEED, what the fuck if you try to compiler me again I will send assassins after your ass
#endif
}

static void private(mouse_callback)(const mouse_cb_param* param) {
    canvas_t* canvas = param->instance;
    const frame_t* frame = get_root(canvas);
    bounding_box* content_box = &canvas->header.content_box;

    if (glfwGetMouseButton(frame->ctx, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        vec4 mpos = {param->x, param->y, 0.0f, 1.0f};
        mpos = mv4_mul(&canvas->inv_model, &mpos);
        if (content_box->x != -1 && content_box->y != -1) Texture(draw_line)(canvas->sprite->tex, canvas->brush.color, content_box->x, content_box->y, mpos.x, mpos.y);
        else Texture(set_pixel)(canvas->sprite->tex, canvas->brush.color, mpos.x, mpos.y);

        content_box->x = mpos.x;
        content_box->y = mpos.y;
        canvas->header.dirty = 3;
    }
    else content_box->x = content_box->y = -1;

}
static void private(keyboard_callback)(const keyboard_cb_param* param) {
    canvas_t* canvas = param->instance;
    camera_t* camera = &canvas->camera;

    u16 bit = 0;
    switch (param->key) {
        case GLFW_KEY_W: bit = CAM_KEY_W; break;
        case GLFW_KEY_S: bit = CAM_KEY_S; break;
        case GLFW_KEY_A: bit = CAM_KEY_A; break;
        case GLFW_KEY_D: bit = CAM_KEY_D; break;
        case GLFW_KEY_Q: bit = CAM_KEY_Q; break;
        case GLFW_KEY_E: bit = CAM_KEY_E; break;
        case GLFW_KEY_R: bit = CAM_KEY_R; break;
        case GLFW_KEY_LEFT_ALT: {
            break;
        }
        case GLFW_KEY_SPACE:
            if (param->action == GLFW_PRESS) Texture(flush)(canvas->sprite->tex, WHITE);
            canvas->header.dirty = 2;
            return;
        default: return;
    }

    if (param->action == GLFW_PRESS || param->action == GLFW_REPEAT) camera->keys |= bit;
    else if (param->action == GLFW_RELEASE) camera->keys &= ~bit;
}
static void private(scroll_callback)(const scroll_cb_param* param) {
    canvas_t* canvas = param->instance;
    const frame_t* frame = get_root(canvas);
    camera_t* camera = &canvas->camera;
    comp_header_t* parent_header = get_header(canvas->header.parent);

#ifndef ZOOM_SPEED
#define ZOOM_SPEED 0.15f
    const f32 factor = expf(param->delta * ZOOM_SPEED);
#undef ZOOM_SPEED
#else
#error For some reason your dumbass also decided to define this macro why do you have to make me want to shove a shotgun barrel up my mouth
#endif
    f64 mx, my;
    glfwGetCursorPos(frame->ctx, &mx, &my);

    const f32 offset_x = (f32)mx - parent_header->content_box.x;
    const f32 offset_y = (f32)my - parent_header->content_box.y;

    const f32 new_zoom = camera->zoom * factor;
    const f32 clamped  = new_zoom < 0.5f ? 0.5f : (new_zoom > 100.0f ? 100.0f : new_zoom);
    const f32 actual_factor = clamped / camera->zoom;

    camera->position.x = offset_x + actual_factor * (camera->position.x - offset_x);
    camera->position.y = offset_y + actual_factor * (camera->position.y - offset_y);
    camera->zoom = clamped;

    parent_header->dirty = 2;
}

static void private(tick)(canvas_t* canvas) {
    const frame_t* frame = get_root(canvas);
    if (frame->focused.instance == canvas && canvas->camera.keys) private(camera_handler)(canvas);
    else canvas->camera.keys = 0;
}

canvas_t* Canvas(new)(void* parent, const u16 width, const u16 height) {
    buf_t buffer = {
        .size = sizeof(canvas_t),
        .tag = MEMTAG_CANVAS
    };
    if (!Buffer(new)(&buffer, true)) return NULL;

    const comp_header_t* parent_header = get_header(parent);

    canvas_t* canvas = buffer.ptr;
    canvas->header.dirty = 2;
    canvas->header.content_box.width = width;
    canvas->header.content_box.height = height;
    canvas->header.content_box.x = -1;
    canvas->header.content_box.y = -1;

    canvas->header.box.x = parent_header->content_box.x;
    canvas->header.box.y = parent_header->content_box.y;
    canvas->header.box.width = parent_header->content_box.width;
    canvas->header.box.height = parent_header->content_box.height;

    canvas->header.parent = parent;

    canvas->sprite = Sprite(new)(get_root(parent), RECT_SHADER);
    if (!canvas->sprite) goto cleanup;
    if (!Sprite(set_texture)(canvas->sprite, width, height, &(style_t){.background = {.type = BG_COLOR, .color = WHITE}})) goto cleanup;

    canvas->camera = Camera(new)();
    canvas->header.mouse = (callback)private(mouse_callback);
    canvas->header.keyboard = (callback)private(keyboard_callback);
    canvas->header.scroll = (callback)private(scroll_callback);
    canvas->header.update = (callback)Canvas(update);
    canvas->header.tick = (callback)private(tick);
    canvas->header.free = (callback)Canvas(del);
    Component(push_node)(parent_header->components, canvas, CANVAS_COMPONENT);
    return canvas;
cleanup:
    if (canvas->sprite) Sprite(del)(canvas->sprite);
    Buffer(del)(&(buf_t){.size = sizeof(canvas_t), .tag = MEMTAG_CANVAS, .ptr = canvas});
    return NULL;
}
void Canvas(del)(canvas_t* canvas) {
    if (!canvas) return;
    Sprite(del)(canvas->sprite);
    Buffer(del)(&(buf_t){.size = sizeof(canvas_t), .tag = MEMTAG_CANVAS, .ptr = canvas});
}
void Canvas(bind)(canvas_t* canvas) {
    if (!canvas) return;
    Sprite(bind)(get_root(canvas), canvas->sprite);
}
void Canvas(set_brush)(canvas_t* canvas, const color_t color, const f32 size) {
    if (!canvas) return;
    canvas->brush.color = color;
    canvas->brush.size = size;
}

void Canvas(update)(canvas_t* canvas) {
    if (!canvas || !canvas->header.dirty) return;

    const frame_t* frame = get_root(canvas);
    comp_header_t* parent_header = get_header(canvas->header.parent);

    Sprite(bind)(frame, canvas->sprite);
    Shader(set_mat4)(canvas->sprite->shader, "projection", true, frame->cache.projection.e);

    if (canvas->header.dirty < 3) {
        const mat4 rotation = m4_rotateZ(rad(Camera(from_angle16)(canvas->camera.roll)));
        const mat4 position = m4_transl(canvas->camera.position.x + parent_header->content_box.x, canvas->camera.position.y + parent_header->content_box.y, 0.0f);
        const mat4 size = m4_transl(canvas->camera.zoom * (f32)canvas->header.content_box.width * 0.5f, canvas->camera.zoom * (f32)canvas->header.content_box.height * 0.5f, 0.0f);
        const mat4 inv_size = m4_transl(-canvas->camera.zoom * (f32)canvas->header.content_box.width * 0.5f, -canvas->camera.zoom * (f32)canvas->header.content_box.height * 0.5f, 0.0f);

        const mat4 scale = m4_scale(canvas->camera.zoom * (f32)canvas->header.content_box.width, canvas->camera.zoom * (f32)canvas->header.content_box.height, 1.0f);
        const mat4 inv_scale = m4_scale(canvas->camera.zoom, canvas->camera.zoom, 1.0f);
        mat4 model = m4_mul(&position, &size);
        model = m4_mul(&model, &rotation);
        model = m4_mul(&model, &inv_size);

        canvas->inv_model = m4_mul(&model, &inv_scale);
        model = m4_mul(&model, &scale);

        canvas->inv_model = m4_inverse(&canvas->inv_model);
        canvas->inv_model = m4_transp(&canvas->inv_model);
        canvas->header.dirty--;

        Shader(set_mat4)(canvas->sprite->shader, "model", true, model.e);
    }
    else canvas->header.dirty = 0;

    glEnable(GL_SCISSOR_TEST);
    glScissor(
        parent_header->content_box.x, frame->header.box.height - parent_header->content_box.y - parent_header->content_box.height,
        parent_header->content_box.width, parent_header->content_box.height
    );
    Mesh(draw)(canvas->sprite->mesh);
    glDisable(GL_SCISSOR_TEST);

    canvas->header.box.x = parent_header->content_box.x;
    canvas->header.box.y = parent_header->content_box.y;
    canvas->header.box.width = parent_header->content_box.width;
    canvas->header.box.height = parent_header->content_box.height;

}