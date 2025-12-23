#include <glfw3.h>
#include <stdio.h>

#include <frame.h>
#include <log.h>
#include <mem.h>
#include <math-utils.h>
#include <panel.h>


#define WIDTH 800
#define HEIGHT 800


int main(void) {
    open_logging("__log__.dat", true);
    set_exitFlag(false);

    frame_t* frame = new_frame(DARK_GRAY, WIDTH, HEIGHT, "Frame");
    printf("entry-frame=%p\n", frame);

    panel_t* panel = new_panel(frame, BLUE, &(bounding_box){200, 100, 400, 600});
    push_comp_node(frame->header.components, panel, PANEL_COMPONENT);

    f32 angle = 0.0f;
    while(!glfwWindowShouldClose(frame->glfw_ctx)) {
        const mat4 projection = m4_ortho(0.0f, (f32)frame->header.box.width, (f32)frame->header.box.height, 0.0f, -1.0f, 1.0f);
        update_frame(frame);

        bind_panel(panel);
        angle += 0.5f;
        if (angle > deg(PI2)) angle -= deg(PI2);
        if (angle < 0.0f) angle += deg(PI2);
        update_panel(panel, &projection, angle);

        frame->header.keyboard(frame);
        // updates canvas buffers
        glfwSwapBuffers(frame->glfw_ctx);
        glfwPollEvents();
    }
    del_panel(panel);
    del_frame(frame);
    print_memtable();

    set_exitFlag(true);
    close_logging();
    return 0;
}
