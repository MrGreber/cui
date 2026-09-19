#include <component_system.h>
#include <memio.h>
#include <frame.h>

#include <stdio.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3.h>
#include <glfw3native.h>

_Thread_local GLFWcursor* __cursors[__COMPONENT_TAG_COUNT__] = { 0 };
const static char* __components_strings__[] = {
    [FRAME_COMPONENT]   = "frame",
    [PANEL_COMPONENT]   = "panel",
    [CAPTION_COMPONENT] = "caption",
    [BUTTON_COMPONENT]  = "button",
    [EDIT_COMPONENT]    = "edit",
    [CANVAS_COMPONENT]  = "canvas"
};

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
    for (u16 i = 0; i < root->count; i++) Component(del_node)(root->nodes[i]);

    comp_header_t* header = get_header(root->component.instance);
    header->vtable->free(root->component.instance);

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
    if (root->capacity >= INT16_MAX) return false;
    const u16 new_capacity = root->capacity << 1;

    buf_t buffer = {
        .size = root->capacity * sizeof(comp_node_t*),
        .tag = MEMTAG_POINTER,
        .ptr = root->nodes
    };
    if (!Buffer(renew)(&buffer, new_capacity)) return false;

    root->capacity = new_capacity;
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
static void Component(poll)(const comp_node_t* node) {
    const comp_header_t* header = get_header(node->component.instance);
    if (!header->parent) return;

    const comp_header_t* parent_header = get_header(header->parent);

    u16 i = 0;
    for (; i < parent_header->components->count && parent_header->components->nodes[i] != node; i++);
    i++;
    for (; i < parent_header->components->count; i++) {
        comp_header_t* sub_header = get_header(parent_header->components->nodes[i]->component.instance);
        sub_header->dirty = is_intersected(&sub_header->box, &parent_header->box);
    }
}
void Component(update)(const comp_node_t* node) {
    if (!node) return;

    const comp_header_t* header = get_header(node->component.instance);
    if (header->vtable->tick) header->vtable->tick(node->component.instance);
    // if (header->dirty) {
    // Component(poll)(node);

    header->vtable->update(node->component.instance);

    for (u16 i = 0; i < node->count; i++) {
        comp_header_t* child_header = get_header(node->nodes[i]->component.instance);
        child_header->dirty = 2; // 1;
        Component(update)(node->nodes[i]);
    }
    // }
    // else {
    //     for (u16 i = 0; i < node->count; i++) {
    //         Component(update)(node->nodes[i]);
    //     }
    // }
}