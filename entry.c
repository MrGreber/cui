#include <frame.h>
#include <log.h>
#include <math-utils.h>
#include <panel.h>
#include <app.h>
#include <edit.h>
#include <button.h>
#include <canvas.h>

#include "glad.h"
#include "hmap.h"

#define WIDTH 800
#define HEIGHT 800

static void __init(app_t* app) {
    frame_t* frame = new_frame(DARK_GRAY, WIDTH, HEIGHT, "Frame");

    style_group_t group = {
        .normal = {
            .init = true,
            .background = {
                .type = BG_COLOR,
                .color = TRANSP
            },
            .border = {
                .color = BLACK,
                .thickness = 1,
                .radius = 0
            }
        }
    };
    panel_t* panel = new_panel(frame, &group, &(bounding_box){200, 100, 600, 600});
    canvas_t* canvas = new_canvas(panel, 400, 600);
    set_brush(canvas, RED, 1);

    group.normal.background.color = WHITE;
    edit_t* edit = new_edit(frame, &group, &(bounding_box){0, 0, 200, 100});
    set_font(edit, __DIR__"\\Resources\\vcr_osd_mono.fnt", BLUE, TRANSP);

    app->frame = frame;
    push_app_var(app, edit);
    push_app_var(app, canvas);
    push_app_var(app, panel);
    print_comp_node(frame->header.components, 0);
}
static bool flag = true;
static void __loop(app_t* app) {
    static f32 angle = 0.0;
    frame_t* frame = app->frame;
    edit_t* edit = get_app_var(app, 0);
    canvas_t* canvas = get_app_var(app, 1);
    panel_t* panel = get_app_var(app, 2);

    const mat4 projection = m4_ortho(0.0f, (f32)frame->header.box.width, (f32)frame->header.box.height, 0.0f, -1.0f, 1.0f);
    update_frame(frame);

    // angle += 0.5f;
    // if (angle >= 360.0f) angle -= 360.0f;

    bind_edit(edit);
    update_edit(edit, &projection, 0.0f);
    bind_panel(panel);
    update_panel(panel, &projection, 0.0f);
    bind_canvas(canvas);
    update_canvas(canvas, &projection);

    if (flag) {
        print_shader(panel->sprite->shader);
        print_shader(canvas->sprite->shader);
        print_shader(edit->sprite->shader);
        flag = false;
    }
}
static void __exit(app_t* app) {
    frame_t* frame = app->frame;
    edit_t* edit = get_app_var(app, 0);
    canvas_t* canvas = get_app_var(app, 1);
    panel_t* panel = get_app_var(app, 2);

    del_edit(edit);
    del_panel(panel);
    del_canvas(canvas);
    del_frame(frame);
}


int main(void) {
    open_logging("__log__.dat", true);
    set_exitFlag(false);

    app_t* app = new_app((app_init_t)__init, (app_loop_t)__loop, (app_exit_t)__exit);
    start_app(app);
    exit_app(app);

    set_exitFlag(true);
    close_logging();
    return 0;
}