#include <event_system.h>
#include <memio.h>
#include <frame.h>
#include <stdio.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3.h>
#include <glfw3native.h>

_Thread_local static GLFWcursor* __cursors[__COMPONENT_TAG_COUNT__] = { 0 };
#define __get_comp_cursor(tag) __cursors[tag]

comp_node_t* Component(new_node)(void* data, const comp_tag tag) {
    buf_t buffer = {
        .size = sizeof(comp_node_t),
        .tag = MEMTAG_COMPONENT_NODE
    };
    if (!Buffer(new)(&buffer, true)) return NULL;

    comp_node_t* tree = buffer.ptr;
    tree->component.instance = data;
    tree->component.tag = tag;

    tree->capacity = 4;
    buffer = (buf_t){
        .size = 4 * sizeof(comp_node_t*),
        .tag = MEMTAG_POINTER
    };
    if (!Buffer(new)(&buffer, true)) goto cleanup;
    tree->nodes = buffer.ptr;

    if (tag == FRAME_COMPONENT && !__cursors[0]) {
        __cursors[0] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        __cursors[1] = __cursors[0];
        __cursors[2] = __cursors[0];
        __cursors[3] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
        __cursors[4] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
        __cursors[5] = load_cursor(__DIR__"\\Resources\\oval.png", 8, 8, 4, 4);
        glfwSetCursor(((frame_t*)data)->ctx, __cursors[0]);
    }

    return tree;
cleanup:
    Buffer(del)(&(buf_t){.size = sizeof(comp_node_t), .tag = MEMTAG_COMPONENT_NODE, .ptr = tree});
    return NULL;
}
void Component(del_node)(comp_node_t* root) {
    if (!root) return;
    for (u64 i = 0; i < root->count; i++) Component(del_node)(root->nodes[i]);
    Buffer(del)(&(buf_t){.size = root->capacity * sizeof(comp_node_t*), .tag = MEMTAG_POINTER, .ptr = root->nodes});
    Buffer(del)(&(buf_t){.size = sizeof(comp_node_t), .tag = MEMTAG_COMPONENT_NODE, .ptr = root});

    for (u16 i = 2; i < __COMPONENT_TAG_COUNT__; i++) {
        if (__cursors[i]) {
            glfwDestroyCursor(__cursors[i]);
            __cursors[i] = NULL;
        }
    }
}
static bool private(resize_tree)(comp_node_t* root) {
    const u64 new_cap = root->capacity << 1;

    buf_t buffer = {
        .size = root->capacity * sizeof(comp_node_t*),
        .tag = MEMTAG_POINTER,
        .ptr = root->nodes
    };
    if (!Buffer(renew)(&buffer, new_cap)) return false;

    root->capacity = new_cap;
    return true;
}
bool Component(push_node)(comp_node_t* root, void* val, const comp_tag tag) {
    if (!root) return false;

    comp_node_t* node = Component(new_node)(val, tag);
    if (!node) return false;

    if (root->count == root->capacity && !private(resize_tree)(root)) goto cleanup;
    root->nodes[root->count++] = node;

    node->root = root->root ? root->root : root;
    comp_header_t* header = get_header(val);
    header->components = node;

    return true;
cleanup:
    Component(del_node)(node);
    return false;
}

const static char* __components_strings__[] = {
    [FRAME_COMPONENT]   = "frame",
    [PANEL_COMPONENT]   = "panel",
    [CAPTION_COMPONENT] = "caption",
    [BUTTON_COMPONENT]  = "button",
    [EDIT_COMPONENT]    = "edit",
    [CANVAS_COMPONENT]  = "canvas"
};

void Component(print_node)(comp_node_t* root, const u64 indent) {
    if (!root) return;

    if (indent) {
        for (u64 i = 0; i < indent - 1; i++) {
            if (root->count > 0) putchar('|');
            putchar('\t');
        }
    }

    if (root->component.tag == FRAME_COMPONENT) printf("%s[%p]\n", __components_strings__[root->component.tag], root->component.instance);
    else printf("|__%s[%p]\n", __components_strings__[root->component.tag], root->component.instance);

    for (u64 i = 0; i < root->count; i++) {
        comp_node_t* node = root->nodes[i];
        for (u64 j = 0; j < indent; j++) {
            if (root->count > 0) putchar('|');
            putchar('\t');
        }
        Component(print_node)(node, indent + 1);

    }
}
void dispatch_event(const comp_node_t* node, event_t* event) {
    if (!node) return;
    if (get_header(node->component.instance)->hide) return;

    frame_t* frame = node->root ? node->root->component.instance : node->component.instance;
    if (!frame->focused.instance) {
        frame->focused.instance = frame;
        frame->focused.tag  = FRAME_COMPONENT;
        frame->hovered.instance = frame;
        frame->hovered.tag  = FRAME_COMPONENT;
    }

    switch (event->tag) {
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
                     dispatch_event(node->nodes[i], event);
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
                 GLFWcursor* desired = __get_comp_cursor(node->component.tag);
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
            for (u64 i = 0; i < node->count; i++) dispatch_event(node->nodes[i], event);

            if (header && header->resize) {
                param->instance = node->component.instance;
                ((callback)header->resize)(param);
            }
            break;
        }
    }
}

void Component(update)(const comp_node_t* node) {
    if (!node) return;

    const comp_header_t* parent_header = get_header(node->component.instance);
    if (parent_header->dirty) {
        for (u64 i = 0; i < node->count; i++) {
            void* child = node->nodes[i]->component.instance;
            comp_header_t* child_header = get_header(child);
            child_header->dirty = 1;
            if (child_header->update) child_header->update(child);
        }
    }
}