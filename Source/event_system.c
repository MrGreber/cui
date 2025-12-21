#include <event_system.h>
#include <mem.h>
#include <frame.h>
#include <stdio.h>

comp_node_t* new_comp_node(void* data, const component_tag tag) {
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
bool push_comp_node(comp_node_t* root, void* val, const component_tag tag) {
    if (!root) return false;

    comp_node_t* node = new_comp_node(val, tag);
    if (!node) return false;

    if (root->count == root->capacity && !__resize_tree(root)) goto cleanup;
    root->nodes[root->count++] = node;

    node->root = root->root ? root->root : root;
    component_header* header = get_header(val);
    header->components = node;

    return true;
cleanup:
    del_comp_node(node);
    return false;
}

const char* __components_strings__[] = {
    "frame",
    "canvas",
    "button"
};

void print_comp_node(comp_node_t* root) {
    if (!root) return;

    if (root->component.tag == FRAME_COMPONENT) printf("%s[%p]\n", __components_strings__[root->component.tag], root->component.data);
    else printf("|__%s[%p]\n", __components_strings__[root->component.tag], root->component.data);

    for (u64 i = 0; i < root->count; i++) {
        comp_node_t* node = root->nodes[i];
        print_comp_node(node);
    }
}
void dispatch_event(const comp_node_t* node, event_t* event) {
    if (!node) return;

    frame_t* frame = node->root ? node->root->component.data : node->component.data;
    if (!frame->focused.data) {
        frame->focused.data = frame;
        frame->focused.tag  = FRAME_COMPONENT;
    }

    switch (event->tag) {
        case __MOUSE_EVENT__: {
            mouse_cb_param* param = &event->param.mouse;
            const bool triggered = param->action || param->button;

            // set the current component to be the focus component and calls mouse component callback
            bool flag = false;
            for (u64 i = 0; i < node->count; i++) {
                const component_header* header = get_header(node->nodes[i]->component.data);

                // Todo make this work with rotation
                if (bounded(param->x, param->y, header->box.x, header->box.y, header->box.width, header->box.height)) {
                    dispatch_event(node->nodes[i], event);
                    flag = true;
                    break;
                }

            }

            if (!flag) {
                const component_header* header = get_header(node->component.data);
                if (triggered) {
                    frame->focused.data = node->component.data;
                    frame->focused.tag = node->component.tag;
                }

                if (!header || !header->mouse) return;
                param->instance = node->component.data;
                ((callback)header->mouse)(param);
            }
            break;
        }
        case __SCROLL_EVENT__: {
            scroll_cb_param* param = &event->param.scroll;
            const component_t* focused = &frame->focused;

            if (!focused->data) return;
            const component_header* header = get_header(focused->data);

            param->instance = focused->data;
            if (header->scroll) ((callback)header->scroll)(param);
            break;
        }
        case __KEYBOARD_EVENT__: {
            keyboard_cb_param* param = &event->param.keyboard;
            const component_t* focused = &frame->focused;

            if (!focused->data || focused->tag == FRAME_COMPONENT) return;
            const component_header* header = get_header(focused->data);

            param->instance = focused->data;
            if (header->keyboard) ((callback)header->keyboard)(param);
            break;
        }
        case __RESIZE_EVENT__: {
            resize_cb_param* param = &event->param.resize;
            const component_header* header = get_header(node->component.data);
            for (u64 i = 0; i < node->count; i++) dispatch_event(node->nodes[i], event);

            if (header && header->resize) {
                param->instance = node->component.data;
                ((callback)header->resize)(param);
            }
            break;
        }
    }
}