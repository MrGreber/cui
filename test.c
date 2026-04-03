#include <memio.h>
#include <frame.h>
#include <log.h>
#include <panel.h>
#include <app.h>
#include <edit.h>
#include <button.h>
#include <canvas.h>
#include <event_system.h>

#define WIDTH 800
#define HEIGHT 800

static bool __init(app_t* app) {
    frame_t* frame = Frame(new)(DARK_GRAY, WIDTH, HEIGHT, "Frame");
    if (!frame) return false;

    style_group_t group = {
        .normal = {
            .init = true,
            .background = {
                .type = BG_TEST,
                //.image = __DIR__"\\Resources\\heisenberg.jpg",
                .mask = MAGENTA
            },
            .border = {
                .color = BLACK,
                .thickness = 1,
                .radius = 0
            },
            .mode = EMBEDDED_POPUP | CAPTION
        },
        .hover = {
            .init = true,
            .background = {
                .mask = {200, 200, 200, 255}
            }
        }
    };
    panel_t* panel = Panel(new)(frame, &group, &(bounding_box){200, 100, 600, 600});
    group.normal.background.type = BG_COLOR;
    group.normal.background.color = WHITE;

    canvas_t* canvas = Canvas(new)(panel, 400, 600);
    Canvas(set_brush)(canvas, RED, 1);

    group.normal.background.color = WHITE;
    group.normal.mode = WRITABLE;
    edit_t* edit = Edit(new)(frame, &group, &(bounding_box){0, 0, 200, 100});
    Font(set)(edit->font, __DIR__"\\Resources\\vcr_osd_mono.fnt", BLUE, TRANSP);
    Edit(set_text)(edit, "shit", 4);
    //button_t* button = new_button(frame, &group, &(bounding_box){0, 700, 100, 100});

    app->frame = frame;
    App(push)(app, panel);
    App(push)(app, canvas);
    App(push)(app, edit);
    //push_app_var(app, button);
    print_comp_node(frame->header.components, 0);
    return true;
}
static void __loop(app_t* app) {
    frame_t* frame = app->frame;
    panel_t* panel = App(get)(app, 0);
    canvas_t* canvas = App(get)(app, 1);
    edit_t* edit = App(get)(app, 2);
    //button_t* button = get_app_var(app, 3);
    const mat4 projection = m4_ortho(0.0f, (f32)frame->header.box.width, (f32)frame->header.box.height, 0.0f, -1.0f, 1.0f);
    Frame(update)(frame);


    // bind_button(button);
    // update_button(button, &projection);
    Panel(bind)(panel);
    Panel(update)(panel, &projection);
    Canvas(bind)(canvas);
    Canvas(update)(canvas, &projection);
    Edit(bind)(edit);
    Edit(update)(edit, &projection);
}
static void __exit(app_t* app) {
    frame_t* frame = app->frame;
    panel_t* panel = App(get)(app, 0);
    canvas_t* canvas = App(get)(app, 1);
    edit_t* edit = App(get)(app, 2);
    //button_t* button = get_app_var(app, 3);

    Panel(del)(panel);
    Canvas(del)(canvas);
    Edit(del)(edit);
    //del_button(button);
    Frame(del)(frame);
}


void test(void) {
    open_logging("__log__.dat", true);
    set_exitFlag(false);

    app_t* app = App(new)((app_init_t)__init, (app_loop_t)__loop, (app_exit_t)__exit);
    if (app) {
        App(start)(app);
        App(exit)(app);
    }
    print_memtable();
    set_exitFlag(true);
    close_logging();
}


