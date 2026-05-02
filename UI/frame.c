#include <frame.h>
#include <camera.h>
#include <event_system.h>
#include <error.h>
#include <memio.h>
#include <shader/ops.h>
#include <geometry/ops.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glad.h>
#include <glfw3.h>
#include <glfw3native.h>
#include <stdio.h>


static bool private(init_glfw)(void) {
    static bool flag = false;
    if (!flag) {
        if (!glfwInit()) {
            logFatal(ERR_GLFW, "Failed to initialize GLFW");
            return false;
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RED_BITS, 8);
        glfwWindowHint(GLFW_GREEN_BITS, 8);
        glfwWindowHint(GLFW_BLUE_BITS, 8);
        glfwWindowHint(GLFW_ALPHA_BITS, 8);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
        flag = true;
    }
    return true;
}
static bool private(init_glad)(void) {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        logError(ERR_GLFW, "Failed to initialize GLAD function.");
        return false;
    }

    static bool flag = false;
    if (!flag) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        flag = true;
    }
    return true;
}

extern void __resize_callback(GLFWwindow* window, const i32 width, const i32 height);
extern void __keyboard_callback(GLFWwindow* window, i32 key, i32 sc, i32 action, i32 modes);
extern void __mouse_movement_callback(GLFWwindow* window, const f64 mouse_x, const f64 mouse_y);
extern void __mouse_button_callback(GLFWwindow* window, const i32 button, const i32 action, const i32 mods);
extern void __scroll_callback(GLFWwindow* window, const f64 x, const f64 scroll_y);


frame_t* Frame(new)(const color_t bg, const u32 width, const u32 height, const char* title) {
    buf_t buffer = {
        .size = sizeof(frame_t),
        .tag = MEMTAG_FRAME,
    };
    if (!Buffer(new)(&buffer, true)) return NULL;

    frame_t* frame = buffer.ptr;

    if (!private(init_glfw)()) goto cleanup;
    frame->ctx = glfwCreateWindow(width, height, title, 0, 0);
    if (!frame->ctx) {
        logError(ERR_GLFW, "Failed to create frame window.");
        goto cleanup;
    }
    frame->header.components = Component(new_node)(frame, FRAME_COMPONENT);
    if (!frame->header.components) {
        logError(ERR_COMPONENT_SYSTEM, "Failed to create component system.");
        goto cleanup;
    }

    glfwMakeContextCurrent(frame->ctx);
    glfwSetFramebufferSizeCallback(frame->ctx, __resize_callback);
    glfwSetKeyCallback(frame->ctx, __keyboard_callback);
    glfwSetCursorPosCallback(frame->ctx, __mouse_movement_callback);
    glfwSetMouseButtonCallback(frame->ctx, __mouse_button_callback);
    glfwSetScrollCallback(frame->ctx, __scroll_callback);
    glfwSetWindowUserPointer(frame->ctx, frame);

    if (!private(init_glad)()) goto cleanup;

    frame->header.box.width = width;
    frame->header.box.height = height;

    frame->header.content_box.width = width;
    frame->header.content_box.height = height;
    frame->header.dirty = 1;

    frame->bg = bg;
    frame->title = (char*)title;
    if (!Shader(new_cache)(frame)) {
        logError(ERR_SHADER_CACHE, "Failed to create shader cache.");
        goto cleanup;
    }
    if (!Mesh(new_cache)(frame)) {
        logError(ERR_MESH_CACHE, "Failed to create mesh cache.");
        goto cleanup;
    }
    frame->cache.projection = m4_ortho(0.0f, (f32)frame->header.box.width, (f32)frame->header.box.height, 0.0f, -1.0f, 1.0f);

    glClearColor(
        u8tof32(bg.r),
        u8tof32(bg.g),
        u8tof32(bg.b),
        u8tof32(bg.a)
    );
    glClear(GL_COLOR_BUFFER_BIT);
    return frame;
cleanup:
    if (frame->header.components) Component(del_node)(frame->header.components);
    Shader(del_cache)(frame);
    Buffer(del)(&(buf_t){.size = sizeof(frame_t), .tag = MEMTAG_FRAME, .ptr = frame});
    glfwTerminate();
    return NULL;
}
void Frame(del)(frame_t* frame) {
    if (!frame) return;
    Shader(del_cache)(frame);
    Mesh(del_cache)(frame);
    if (frame->header.components) Component(del_node)(frame->header.components);
    Buffer(del)(&(buf_t){.size = sizeof(frame_t), .tag = MEMTAG_FRAME, .ptr = frame});
    glfwTerminate();
}

void Frame(update)(frame_t* frame) {
    if (!frame) return;

    static char caption[64] = { 0 };
    static u32 frame_count = 0;
    static f64 acc = 0.0;
    Stopwatch(update)(&frame->stopwatch);
    acc += frame->stopwatch.delta;
    frame_count++;

    if (acc >= 1.0f) {
        const f64 fps = (f64)frame_count / acc;
        sprintf_s(caption, sizeof(caption), "%s-FPS: %.2f", frame->title, fps);
        acc = 0.0;
        frame_count = 0;
    }
    glfwSetWindowTitle(frame->ctx, caption);
    if (frame->header.dirty) {
        Frame(clear)(frame, &frame->header.box);
        frame->header.dirty = 0;
    }
}
void Frame(set_position)(frame_t* frame, const u16 x, const u16 y) {
    if (!frame) return;
    glfwSetWindowPos(frame->ctx, x, y);
}

void Frame(set_flag)(frame_t* frame, const u8 field) {
    if (!frame) return;

    switch (field) {
        case FRAME_HIDE: {
            if (!frame->header.hide) glfwHideWindow(frame->ctx);
            else glfwShowWindow(frame->ctx);
            frame->header.hide ^= 1;
            break;
        }
    }
}

void Frame(clear)(const frame_t* frame, const bounding_box* box) {
    const color_t bg = frame->bg;
    glEnable(GL_SCISSOR_TEST);
    glScissor(box->x, frame->header.box.height - box->y - box->height, box->width, box->height);
    glClearColor(
        u8tof32(bg.r),
        u8tof32(bg.g),
        u8tof32(bg.b),
        u8tof32(bg.a)
    );
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}