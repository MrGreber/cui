#include <frame.h>
#include <mem.h>
#include <camera.h>
#include <../Recycle/canvas.h>
#include <../Recycle/button.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glad.h>
#include <glfw3.h>
#include <glfw3native.h>
#include <stdio.h>

void __resize_callback(GLFWwindow* window, const i32 width, const i32 height) {
    if (width <= 0 || height <= 0) return;
    glViewport(0, 0, width, height);

    frame_t* frame = glfwGetWindowUserPointer(window);
    if (frame->header.box.width == width && frame->header.box.height == height) return;
    frame->header.box.width = (u32)width;
    frame->header.box.height = (u32)height;

    event_t event = {
        .param.resize = {.height = height, .width = width},
        .tag = __RESIZE_EVENT__
    };
    const comp_node_t* root = frame->header.components;
    dispatch_event(root, &event);
}
bool __check_press(const frame_t* frame, const u32 key) {
    static bool key_pressed[GLFW_KEY_LAST] = { 0 };
    if (glfwGetKey(frame->glfw_ctx, key) == GLFW_PRESS && !key_pressed[key]) {
        key_pressed[key] = true;
        return true;
    }
    if (glfwGetKey(frame->glfw_ctx, key) == GLFW_RELEASE) {
        key_pressed[key] = false;
    }
    return false;
}
void __keyboard_input(const frame_t* frame) {
    event_t event = {.tag = __KEYBOARD_EVENT__};
    const comp_node_t* root = frame->header.components;
    dispatch_event(root, &event);
}
void __keyboard_callback(GLFWwindow* window, const i32 key, const i32 sc, const i32 action, const i32 mods) {
    static bool wireframe_mode = false;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        wireframe_mode = !wireframe_mode;
        glPolygonMode(GL_FRONT_AND_BACK, wireframe_mode ? GL_LINE : GL_FILL);
    }
}
void __mouse_movement_callback(GLFWwindow* window, const f64 mouse_x, const f64 mouse_y) {
    event_t event = {
        .param.mouse = {.x = mouse_x, .y = mouse_y},
        .tag = __MOUSE_EVENT__
    };
    const frame_t* frame = glfwGetWindowUserPointer(window);
    const comp_node_t* root = frame->header.components;
    dispatch_event(root, &event);
}
void __mouse_button_callback(GLFWwindow* window, const i32 button, const i32 action, const i32 mods) {
    f64 mouse_x = 0.0, mouse_y = 0.0;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    event_t event = {
        .param.mouse = {.x = mouse_x, .y = mouse_y, .button = button, .action = action, .mods = mods},
        .tag = __MOUSE_EVENT__
    };
    const frame_t* frame = glfwGetWindowUserPointer(window);
    const comp_node_t* root = frame->header.components;
    dispatch_event(root, &event);
}
void __scroll_callback(GLFWwindow* window, const f64 x, const f64 scroll_y) {
    event_t event = {
        .param.scroll = {.delta = scroll_y},
        .tag = __SCROLL_EVENT__
    };
    const frame_t* frame = glfwGetWindowUserPointer(window);
    const comp_node_t* root = frame->header.components;
    dispatch_event(root, &event);
}