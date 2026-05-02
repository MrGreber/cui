#include <frame.h>
#include <memio.h>
#include <camera.h>
#include <event_system.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glad.h>
#include <glfw3.h>

void __resize_callback(GLFWwindow* window, const i32 width, const i32 height) {
    if (width <= 0 || height <= 0) return;
    glViewport(0, 0, width, height);

    frame_t* frame = glfwGetWindowUserPointer(window);
    if (frame->header.box.width == width && frame->header.box.height == height) return;

    i32 delta_width = width - frame->header.box.width;
    i32 delta_height = height - frame->header.box.height;

    frame->header.box.width = (u32)width;
    frame->header.box.height = (u32)height;

    frame->cache.projection = m4_ortho(0.0f, (f32)frame->header.box.width, (f32)frame->header.box.height, 0.0f, -1.0f, 1.0f);
    event_t event = {
        .param.resize = {.height = delta_height, .width = delta_width},
        .tag = __RESIZE_EVENT__
    };
    const comp_node_t* root = frame->header.components;
    frame->header.dirty = 1;
    dispatch_event(root, &event);
}
void __keyboard_callback(GLFWwindow* window, const i32 key, const i32 sc, const i32 action, const i32 modes) {
    static bool wireframe_mode = false;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        wireframe_mode = !wireframe_mode;
        glPolygonMode(GL_FRONT_AND_BACK, wireframe_mode ? GL_LINE : GL_FILL);
    }
    const frame_t* frame = glfwGetWindowUserPointer(window);
    event_t event = {
        .param.keyboard = {.key = key, .scancode = sc, .action = action, .modes = modes},
        .tag = __KEYBOARD_EVENT__
    };
    const comp_node_t* root = frame->header.components;
    dispatch_event(root, &event);
}
void __mouse_movement_callback(GLFWwindow* window, const f64 mouse_x, const f64 mouse_y) {
    event_t event = {
        .param.mouse = {.x = mouse_x, .y = mouse_y, .action = -1, .button = -1},
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