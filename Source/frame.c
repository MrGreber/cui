#include <frame.h>
#include <mem.h>
#include <log.h>
#include <camera.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glad.h>
#include <glfw3.h>
#include <glfw3native.h>
#include <stdio.h>

static bool __init_glfw(void) {
    static bool flag = false;
    if (!flag) {
        if (!glfwInit()) {
            logFatal("init_glfw - Failed to initialize GLFW");
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
static bool __init_glad(void) {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        logError("__init_glad - Failed to initialize GLAD function.");
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

#define FLAG_DEFAULT_STATE 1

frame_t* new_frame(const color_t bg, const u32 width, const u32 height, const char* title) {
    buf_t buffer = {
        .size = sizeof(frame_t),
        .tag = MEMTAG_FRAME,
    };
    if (!new_buf(&buffer, true)) return NULL;

    frame_t* frame = buffer.ptr;

    if (!__init_glfw()) goto cleanup;
    frame->ctx = glfwCreateWindow(width, height, title, 0, 0);
    if (!frame->ctx) {
        logError("new_frame - Failed to create frame window.");
        goto cleanup;
    }
    frame->header.components = new_comp_node(frame, FRAME_COMPONENT);
    if (!frame->header.components) {
        logError("new_frame - Failed to create component system.");
        goto cleanup;
    }

    glfwMakeContextCurrent(frame->ctx);
    glfwSetFramebufferSizeCallback(frame->ctx, __resize_callback);

    if (!__init_glad()) goto cleanup;

    frame->header.box.width = width;
    frame->header.box.height = height;
    frame->bg = bg;
    frame->title = (char*)title;
    frame->flags = FLAG_DEFAULT_STATE;

    glfwSetKeyCallback(frame->ctx, __keyboard_callback);
    glfwSetCursorPosCallback(frame->ctx, __mouse_movement_callback);
    glfwSetMouseButtonCallback(frame->ctx, __mouse_button_callback);
    glfwSetScrollCallback(frame->ctx, __scroll_callback);

    glfwSetWindowUserPointer(frame->ctx, frame);
    return frame;
cleanup:
    if (frame->header.components) del_comp_node(frame->header.components);
    del_buf(&(buf_t){.size = sizeof(frame_t), .tag = MEMTAG_FRAME, .ptr = frame});
    glfwTerminate();
    return NULL;
}
void del_frame(frame_t* frame) {
    if (!frame) return;
    if (frame->header.components) del_comp_node(frame->header.components);
    del_buf(&(buf_t){.size = sizeof(frame_t), .tag = MEMTAG_FRAME, .ptr = frame});
    glfwTerminate();
}

void update_frame(const frame_t* frame) {
    if (!frame) return;

    static char caption[64] = { 0 };
    static u32 frame_count = 0;
    static f64 acc = 0.0;
    const f64 dt = get_deltaTime();
    acc += dt;
    frame_count++;

    if (acc >= 1.0f) {
        const f64 fps = frame_count / acc;
        sprintf_s(caption, sizeof(caption), "%s-FPS: %.2f", frame->title, fps);
        acc = 0.0f;
        frame_count = 0;
    }
    glfwSetWindowTitle(frame->ctx, caption);

    const color_t bg = frame->bg;
    glViewport(0, 0, frame->header.box.width, frame->header.box.height);
    // clears the window to a color
    glClearColor(
        byte_to_float(bg.r),
        byte_to_float(bg.g),
        byte_to_float(bg.b),
        byte_to_float(bg.a)
    );
    glClear(GL_COLOR_BUFFER_BIT);

}
void set_frame_position(frame_t* frame, const u16 x, const u16 y) {
    if (!frame) return;

    glfwSetWindowPos(frame->ctx, x, y);
}

void set_frame_flag(frame_t* frame, const frame_flag field) {
    if (!frame) return;
    if (sizeof(frame->flags) <= field) return;

    const byte bit = 1 << field;
    switch (field) {
        case HIDE_FLAG: {
            if (frame->flags & bit) {
                glfwHideWindow(frame->ctx);
                frame->flags ^= bit;
            }
            else {
                glfwShowWindow(frame->ctx);
                frame->flags ^= bit;
            }
            break;
        }
    }
}