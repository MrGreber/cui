#include <frame.h>
#include <events.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3.h>
#include <glfw3native.h>

extern _Thread_local GLFWcursor* __cursors[__COMPONENT_TAG_COUNT__];

static void private(dispatch_event)(const comp_node_t* node, event_t* event) {
    if (!node) return;
    if (get_header(node->component.instance)->hide) return;

    frame_t* frame = node->root ? node->root->component.instance : node->component.instance;
    if (!frame->focused.instance) {
        frame->focused.instance = frame;
        frame->focused.tag  = FRAME_COMPONENT;
        frame->hovered.instance = frame;
        frame->hovered.tag  = FRAME_COMPONENT;
    }

    switch (event->type) {
        case __MOUSE_EVENT__: {
            mouse_cb_param* param = &event->param.mouse;

            if (frame->captured.instance) {
                const comp_header_t* header = get_header(frame->captured.instance);

                param->instance = frame->captured.instance;
                if (header && header->mouse) ((callback)header->mouse)(param);
                return;
            }

             const bool triggered = (param->action == GLFW_PRESS || param->action == GLFW_RELEASE);

             // set the current component to be the focus component and calls mouse component callback
             bool flag = false;
             for (u64 i = 0; i < node->count; i++) {
                 const comp_header_t* header = get_header(node->nodes[i]->component.instance);

                 if (is_bounded(&header->box, param->x, param->y)) {
                     private(dispatch_event)(node->nodes[i], event);
                     flag = true;
                     break;
                 }
             }
             if (!flag) {
                 frame->hovered.instance = node->component.instance;
                 frame->hovered.tag = node->component.tag;

                 const comp_header_t* header = get_header(node->component.instance);
                 if (triggered) {
                     frame->focused.instance = node->component.instance;
                     frame->focused.tag = node->component.tag;
                 }

                 // toggles between different cursors for each component
                 GLFWcursor* desired = __cursors[node->component.tag];
                 if (frame->cursor != desired) {
                     glfwSetCursor(frame->ctx, desired);
                     frame->cursor = desired;
                 }

                 if (!header || !header->mouse) return;
                 param->instance = node->component.instance;
                 ((callback)header->mouse)(param);
            }
            break;
        }
        case __SCROLL_EVENT__: {
            scroll_cb_param* param = &event->param.scroll;
            const comp_t* focused = &frame->focused;

            if (!focused->instance) return;
            const comp_header_t* header = get_header(focused->instance);

            param->instance = focused->instance;
            if (header->scroll) ((callback)header->scroll)(param);
            break;
        }
        case __KEYBOARD_EVENT__: {
            keyboard_cb_param* param = &event->param.keyboard;
            const comp_t* focused = &frame->focused;

            if (!focused->instance || focused->tag == FRAME_COMPONENT) return;
            const comp_header_t* header = get_header(focused->instance);

            param->instance = focused->instance;
            if (header->keyboard) ((callback)header->keyboard)(param);
            break;
        }
        case __RESIZE_EVENT__: {
            resize_cb_param* param = &event->param.resize;
            const comp_header_t* header = get_header(node->component.instance);
            for (u64 i = 0; i < node->count; i++) private(dispatch_event)(node->nodes[i], event);

            if (header && header->resize) {
                param->instance = node->component.instance;
                ((callback)header->resize)(param);
            }
            break;
        }
    }
}
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
        .type = __RESIZE_EVENT__
    };
    const comp_node_t* root = frame->header.components;
    frame->header.dirty = 1;
    private(dispatch_event)(root, &event);
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
        .type = __KEYBOARD_EVENT__
    };
    const comp_node_t* root = frame->header.components;
    private(dispatch_event)(root, &event);
}
void __mouse_movement_callback(GLFWwindow* window, const f64 mouse_x, const f64 mouse_y) {
    event_t event = {
        .param.mouse = {.x = (i32)mouse_x, .y = (i32)mouse_y, .action = -1, .button = -1},
        .type = __MOUSE_EVENT__
    };
    const frame_t* frame = glfwGetWindowUserPointer(window);
    const comp_node_t* root = frame->header.components;
    private(dispatch_event)(root, &event);
}
void __mouse_button_callback(GLFWwindow* window, const i32 button, const i32 action, const i32 mods) {
    f64 mouse_x = 0.0, mouse_y = 0.0;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    event_t event = {
        .param.mouse = {.x = mouse_x, .y = mouse_y, .button = button, .action = action, .mods = mods},
        .type = __MOUSE_EVENT__
    };
    const frame_t* frame = glfwGetWindowUserPointer(window);
    const comp_node_t* root = frame->header.components;
    private(dispatch_event)(root, &event);
}
void __scroll_callback(GLFWwindow* window, const f64 x, const f64 scroll_y) {
    event_t event = {
        .param.scroll = {.delta = scroll_y},
        .type = __SCROLL_EVENT__
    };
    const frame_t* frame = glfwGetWindowUserPointer(window);
    const comp_node_t* root = frame->header.components;
    private(dispatch_event)(root, &event);
}