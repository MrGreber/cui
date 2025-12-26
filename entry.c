#include <stdio.h>

#include <frame.h>
#include <log.h>
#include <mem.h>
#include <math-utils.h>
#include <panel.h>
#include <app.h>

#define WIDTH 800
#define HEIGHT 800

static void __init(app_t* app) {
    frame_t* frame = new_frame(DARK_GRAY, WIDTH, HEIGHT, "Frame");

    style_group_t group = {
        .normal = {
            .init = true,
            .background.color = BLUE,
            .border = {
                .color = RED,
                .thickness = 30,
                .radius = 1
            }
        }
    };
    panel_t* panel1 = new_panel(frame, &group, &(bounding_box){200, 100, 400, 600});
    push_comp_node(frame->header.components, panel1, PANEL_COMPONENT);
    panel_t* panel2 = new_panel(frame, &group, &(bounding_box){0, 0, 100, 100});
    push_comp_node(frame->header.components, panel2, PANEL_COMPONENT);

    app->vars_count = 2;
    buf_t buffer = {
        .size = sizeof(void*) * app->vars_count,
        .tag = MEMTAG_POINTER
    };
    if (!new_buf(&buffer, true)) {
        return;
    }

    app->frame = frame;
    app->vars = buffer.ptr;
    ((u64*)app->vars)[0] = (u64)panel1;
    ((u64*)app->vars)[1] = (u64)panel2;
}

static void __loop(app_t* app) {
    static f32 angle = 0.0;
    const frame_t* frame = app->frame;
    panel_t* panel1 = (panel_t*)((u64*)app->vars)[0];
    panel_t* panel2 = (panel_t*)((u64*)app->vars)[1];

    const mat4 projection = m4_ortho(0.0f, (f32)frame->header.box.width, (f32)frame->header.box.height, 0.0f, -1.0f, 1.0f);
    update_frame(frame);

    // angle += 0.5f;
    // if (angle > deg(PI2)) angle -= deg(PI2);
    // if (angle < 0.0f) angle += deg(PI2);
    bind_panel(panel1);
    update_panel(panel1, &projection, angle);
    bind_panel(panel2);
    update_panel(panel2, &projection, angle);
}

static void __exit(app_t* app) {
    frame_t* frame = app->frame;
    panel_t* panel1 = (panel_t*)((u64*)app->vars)[0];
    panel_t* panel2 = (panel_t*)((u64*)app->vars)[1];

    del_panel(panel1);
    del_panel(panel2);
    del_frame(frame);
    del_buf(&(buf_t){.ptr = app->vars, .tag = MEMTAG_POINTER, .size = sizeof(void*) * app->vars_count});
}

int main(void) {
    open_logging("__log__.dat", true);
    set_exitFlag(false);

    app_t* app = new_app(__init, __loop, __exit);
    start_app(app);
    exit_app(app);

    set_exitFlag(true);
    close_logging();
    return 0;
}
