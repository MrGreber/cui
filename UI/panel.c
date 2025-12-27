#include <panel.h>
#include <mem.h>
#include <event_system.h>
#include <frame.h>
#include <math-utils.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glad.h>
#include <glfw3.h>
#include <glfw3native.h>

#include "stb_image.h"


static void __default_mouse_movement_callback(const mouse_cb_param* param) {
    panel_t* panel = param->instance;
    frame_t* frame = ((comp_node_t*)panel->header.components)->root->component.data;
    //printf("frame=%p\n", frame);
}
static void __default_resize_callback(const resize_cb_param* param) {
    panel_t* panel = param->instance;
    // panel->header.box.width += param->width;
    // panel->header.box.height += param->height;

}

panel_t* new_panel(void* parent, style_group_t* group, const bounding_box* box, const char* path) {
    buf_t buffer = {
        .size = sizeof(panel_t),
        .tag = MEMTAG_PANEL
    };
    if (!new_buf(&buffer, true)) return NULL;

    i32 width, height;
    u16 format;
    const color_t* data = load_texture(path, &width, &height, &format);
    if (data == NULL) goto cleanup;

    panel_t* panel = buffer.ptr;
    panel->header.box.x = box->x;
    panel->header.box.y = box->y;
    panel->header.box.width = width;
    panel->header.box.height = height;
    panel->parent = parent;
    if (group->normal.init) memcpy_s(&panel->styles.normal, sizeof(style_t), &group->normal, sizeof(style_t));
    if (group->hover.init) memcpy_s(&panel->styles.hover, sizeof(style_t), &group->hover, sizeof(style_t));

    panel->tex = new_texture(data, width, height, format);
    stbi_image_free((byte*)data);

    if (!panel->tex) goto cleanup;
    //flush_texture(panel->tex, panel->styles.normal.background.color);

    style_t* normal_style = NULL,* hover_style = NULL;
    if (group->normal.init) normal_style = &group->normal;
    if (group->hover.init) hover_style = &group->hover;

    const u32 max = max(height, width);
    buffer = (buf_t){
        .size = sizeof(color_t) * normal_style->border.thickness * max,
        .tag = MEMTAG_COLOR,
        .ptr = NULL
    };
    if (!new_buf(&buffer, false)) goto cleanup;
    color_t* hori_border = buffer.ptr;
    aligned_memset(
        (u32*)hori_border,
        normal_style->border.color.hex,
        normal_style->border.thickness * max
    );
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, normal_style->border.thickness, GL_RGBA, GL_UNSIGNED_BYTE, hori_border);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, height - normal_style->border.thickness, width, normal_style->border.thickness, GL_RGBA, GL_UNSIGNED_BYTE, hori_border);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, normal_style->border.thickness, height, GL_RGBA, GL_UNSIGNED_BYTE, hori_border);
    glTexSubImage2D(GL_TEXTURE_2D, 0, width - normal_style->border.thickness, 0, normal_style->border.thickness, height, GL_RGBA, GL_UNSIGNED_BYTE, hori_border);
    del_buf(&buffer);

    panel->sprite = new_sprite("__panel__");
    if (!panel->sprite) goto cleanup;


    panel->header.mouse = __default_mouse_movement_callback;
    panel->header.resize =  __default_resize_callback;
    return panel;
cleanup:
    if (panel->sprite) del_sprite(panel->sprite);
    if (panel->tex) del_texture(panel->tex);
    del_buf(&(buf_t){.size = sizeof(panel_t), .tag = MEMTAG_PANEL, .ptr = panel});
    return NULL;
}
void del_panel(panel_t* panel) {
    if (!panel) return;
    if (panel->sprite) del_sprite(panel->sprite);
    if (panel->tex) del_texture(panel->tex);
    del_buf(&(buf_t){.size = sizeof(panel_t), .tag = MEMTAG_PANEL, .ptr = panel});
}
void bind_panel(const panel_t* panel) {
    if (!panel) return;
    bind_sprite(panel->sprite);
    bind_texture(panel->tex);
}

void update_panel(panel_t* panel, const mat4* projection, const f32 angle) {
    if (!panel) return;

    const mat4 rotation = m4_rotateZ(rad(angle));
    const mat4 scale = m4_scale((f32)panel->header.box.width, (f32)panel->header.box.height, 1.0f);
    const mat4 position = m4_transl((f32)panel->header.box.x, (f32)panel->header.box.y, 0.0f);
    const mat4 size = m4_transl((f32)panel->header.box.width * 0.5f, (f32)panel->header.box.height * 0.5f, 0.0f);
    const mat4 inv_size = m4_transl(-(f32)panel->header.box.width * 0.5f, -(f32)panel->header.box.height * 0.5f, 0.0f);

    mat4 model = m4_mul(&position, &size);
    model = m4_mul(&model, &rotation);
    model = m4_mul(&model, &inv_size);
    model = m4_mul(&model, &scale);

    set_mat4_uniform(panel->sprite->shader, "projection", true, projection->e);
    set_mat4_uniform(panel->sprite->shader, "model", true, model.e);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}