#include <panel.h>
#include <mem.h>
#include <event_system.h>
#include <frame.h>
#include <math-utils.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <complex.h>
#include <glad.h>
#include <glfw3.h>
#include <glfw3native.h>
#include <immintrin.h>
#include <stdio.h>



static void __default_mouse_movement_callback(const mouse_cb_param* param) {
    panel_t* panel = param->instance;
    frame_t* frame = ((comp_node_t*)panel->header.components)->root->component.data;
    //printf("frame=%p\n", frame);
}
static void __default_resize_callback(const resize_cb_param* param) {
    panel_t* panel = param->instance;


}

panel_t* new_panel(void* parent, style_group_t* group, const bounding_box* box) {
    buf_t buffer = {
        .size = sizeof(panel_t),
        .tag = MEMTAG_PANEL
    };
    if (!new_buf(&buffer, true)) return NULL;

    panel_t* panel = buffer.ptr;
    panel->header.box.x = box->x;
    panel->header.box.y = box->y;
    panel->header.box.width = box->width;
    panel->header.box.height = box->height;
    panel->parent = parent;
    if (group->normal.init)
        memcpy_s(&panel->styles.normal, sizeof(style_t), &group->normal, sizeof(style_t));
    if (group->hover.init)
        memcpy_s(&panel->styles.hover, sizeof(style_t), &group->hover, sizeof(style_t));

    panel->tex = new_texture(box->width, box->height);
    if (!panel->tex) goto cleanup;
    flush_texture(panel->tex, panel->header.box.width, panel->header.box.height, panel->styles.normal.background.color);

    style_t* normal_style = NULL,* hover_style = NULL;
    if (group->normal.init) normal_style = &group->normal;
    if (group->hover.init) hover_style = &group->hover;

    buffer = (buf_t){
        .size = sizeof(color_t) * normal_style->border.thickness * box->height,
        .tag = MEMTAG_COLOR,
        .ptr = NULL
    };
    if (!new_buf(&buffer, false)) goto cleanup;

    color_t* hori_border = buffer.ptr;
    u32 i = 0;
    for (; i + 8 <= normal_style->border.thickness * box->height; i += 8) {
        const __m256i vc = _mm256_set1_epi32(normal_style->border.color.hex);
        _mm256_storeu_epi32(&hori_border[i], vc);
    }
    for (; i < normal_style->border.thickness * box->height; i++) hori_border[i] = normal_style->border.color;


    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, box->width, normal_style->border.thickness, GL_RGBA, GL_UNSIGNED_BYTE, hori_border);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, box->height - normal_style->border.thickness, box->width, normal_style->border.thickness, GL_RGBA, GL_UNSIGNED_BYTE, hori_border);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, normal_style->border.thickness, box->height, GL_RGBA, GL_UNSIGNED_BYTE, hori_border);
    glTexSubImage2D(GL_TEXTURE_2D, 0, box->width - normal_style->border.thickness, 0, normal_style->border.thickness, box->height, GL_RGBA, GL_UNSIGNED_BYTE, hori_border);
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