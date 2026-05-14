#include <memio.h>
#include <frame.h>
#include <error.h>
#include <panel.h>
#include <app.h>
#include <edit.h>
#include <button.h>
#include <canvas.h>
#include <component_system.h>

#define WIDTH 800
#define HEIGHT 800

static bool __init(app_t* app) {
    frame_t* frame = Frame(new)(DARK_GRAY, WIDTH, HEIGHT, "Frame");
    if (!frame) return false;

    linear_grad_t gradient = {
        .metadata = {
            .colors = (color_t[4]){ RED , BLUE , MAGENTA, GREEN},
            .positions = (f32[4]){0.0f, 0.2f, 0.8f, 1.0f},
            .count = 4
        }
    };
    style_group_t group = {
        .normal = {
            .init = true,
            .background = {
                .type = BG_TEST,
                //.linear_gradient = &gradient,
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
    //button_t* button = Button(new)(frame, &group, &(bounding_box){0, 700, 100, 100});

    app->frame = frame;
    Component(print_node)(frame->header.components, 0);
    return true;
}

// Todo: create unit tests
void test(void) {
    Error(init)(NULL);
    app_t* app = App(new)((app_init_t)__init, NULL, NULL);
    if (app) {
        App(start)(app);
        App(exit)(app);
    }
    Error(dump)();
    print_memtable();
}


