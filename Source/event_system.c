#include <event_system.h>
#include <mem.h>
#include <frame.h>
#include <stdio.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3.h>
#include <glfw3native.h>

#include "edit.h"


static GLFWcursor* __cursors[__COMPONENT_TAG_COUNT__] = { 0 };
#define __get_comp_cursor(tag) __cursors[tag]

comp_node_t* new_comp_node(void* data, const comp_tag tag) {
    buf_t buffer = {
        .size = sizeof(comp_node_t),
        .tag = MEMTAG_COMPONENT_NODE
    };
    if (!new_buf(&buffer, true)) return NULL;

    comp_node_t* tree = buffer.ptr;
    tree->component.data = data;
    tree->component.tag = tag;

    tree->capacity = 4;
    buffer = (buf_t){
        .size = 4 * sizeof(comp_node_t*),
        .tag = MEMTAG_POINTER
    };
    if (!new_buf(&buffer, true)) goto cleanup;
    tree->nodes = buffer.ptr;

    if (tag == FRAME_COMPONENT && !__cursors[0]) {
        __cursors[0] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        __cursors[1] = __cursors[0];
        __cursors[2] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
        __cursors[3] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
        __cursors[4] = load_cursor(__DIR__"\\Resources\\oval.png", 8, 8, 4, 4);
        glfwSetCursor(((frame_t*)data)->ctx, __cursors[0]);
    }

    return tree;
cleanup:
    del_buf(&(buf_t){.size = sizeof(comp_node_t), .tag = MEMTAG_COMPONENT_NODE, .ptr = tree});
    return NULL;
}
void del_comp_node(comp_node_t* root) {
    if (!root) return;

    for (u64 i = 0; i < root->count; i++) del_comp_node(root->nodes[i]);
    del_buf(&(buf_t){.size = root->capacity * sizeof(comp_node_t*), .tag = MEMTAG_POINTER, .ptr = root->nodes});
    del_buf(&(buf_t){.size = sizeof(comp_node_t), .tag = MEMTAG_COMPONENT_NODE, .ptr = root});

    for (u16 i = 1; i < __COMPONENT_TAG_COUNT__; i++) {
        if (__cursors[i]) {
            glfwDestroyCursor(__cursors[i]);
            __cursors[i] = NULL;
        }
    }
}
static bool __resize_tree(comp_node_t* root) {
    const u64 new_cap = root->capacity << 1;

    buf_t buffer = {
        .size = root->capacity * sizeof(comp_node_t*),
        .tag = MEMTAG_POINTER,
        .ptr = root->nodes
    };
    if (!renew_buf(&buffer, new_cap)) return false;

    root->capacity = new_cap;
    return true;
}
bool push_comp_node(comp_node_t* root, void* val, const comp_tag tag) {
    if (!root) return false;

    comp_node_t* node = new_comp_node(val, tag);
    if (!node) return false;

    if (root->count == root->capacity && !__resize_tree(root)) goto cleanup;
    root->nodes[root->count++] = node;

    node->root = root->root ? root->root : root;
    comp_header_t* header = get_header(val);
    header->components = node;

    return true;
cleanup:
    del_comp_node(node);
    return false;
}

const char* __components_strings__[] = {
    "frame",
    "canvas",
    "button",
    "panel",
    "edit"
};

void print_comp_node(comp_node_t* root, u64 indent) {
    if (!root) return;

    if (indent) {
        for (u64 i = 0; i < indent - 1; i++) putc('\t', stdout);
    }

    if (root->component.tag == FRAME_COMPONENT) printf("%s[%p]\n", __components_strings__[root->component.tag], root->component.data);
    else printf("|__%s[%p]\n", __components_strings__[root->component.tag], root->component.data);

    for (u64 i = 0; i < root->count; i++) {
        comp_node_t* node = root->nodes[i];
        print_comp_node(node, indent + 1);

    }
}
void dispatch_event(const comp_node_t* node, event_t* event) {
    if (!node) return;

    frame_t* frame = node->root ? node->root->component.data : node->component.data;
    if (!frame->focused.data) {
        frame->focused.data = frame;
        frame->focused.tag  = FRAME_COMPONENT;
        frame->hovered.data = frame;
        frame->hovered.tag  = FRAME_COMPONENT;
    }

    switch (event->tag) {
        case __MOUSE_EVENT__: {
            mouse_cb_param* param = &event->param.mouse;
            const bool triggered = param->action || param->button;

            // set the current component to be the focus component and calls mouse component callback
            bool flag = false;
            for (u64 i = 0; i < node->count; i++) {
                const comp_header_t* header = get_header(node->nodes[i]->component.data);

                if (bounded(param->x, param->y, header->box.x, header->box.y, header->box.width, header->box.height)) {
                    dispatch_event(node->nodes[i], event);
                    flag = true;
                    break;
                }

            }
            if (!flag) {
                frame->hovered.data = node->component.data;
                frame->hovered.tag = node->component.tag;

                const comp_header_t* header = get_header(node->component.data);
                if (triggered) {
                    frame->focused.data = node->component.data;
                    frame->focused.tag = node->component.tag;
                }

                // toggles between different cursors for each component
                GLFWcursor* desired = __get_comp_cursor(node->component.tag);
                if (frame->cursor != desired) {
                    glfwSetCursor(frame->ctx, desired);
                    frame->cursor = desired;
                }

                if (!header || !header->mouse) return;
                param->instance = node->component.data;
                ((callback)header->mouse)(param);

            }
            break;
        }
        case __SCROLL_EVENT__: {
            scroll_cb_param* param = &event->param.scroll;
            const comp_t* focused = &frame->focused;

            if (!focused->data) return;
            const comp_header_t* header = get_header(focused->data);

            param->instance = focused->data;
            if (header->scroll) ((callback)header->scroll)(param);
            break;
        }
        case __KEYBOARD_EVENT__: {
            keyboard_cb_param* param = &event->param.keyboard;
            const comp_t* focused = &frame->focused;

            if (!focused->data || focused->tag == FRAME_COMPONENT) return;
            const comp_header_t* header = get_header(focused->data);

            param->instance = focused->data;
            if (header->keyboard) ((callback)header->keyboard)(param);
            break;
        }
        case __RESIZE_EVENT__: {
            resize_cb_param* param = &event->param.resize;
            const comp_header_t* header = get_header(node->component.data);
            for (u64 i = 0; i < node->count; i++) dispatch_event(node->nodes[i], event);

            if (header && header->resize) {
                param->instance = node->component.data;
                ((callback)header->resize)(param);
            }
            break;
        }
    }
}