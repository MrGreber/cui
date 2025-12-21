#include <stdio.h>
#include <stdlib.h>
#include <glfw3.h>

#include <frame.h>
#include <log.h>
#include <mem.h>

#define WIDTH 800
#define HEIGHT 800


int main(void) {
    open_logging("__log__.dat", true);
    set_exitFlag(false);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    frame_t* frame = new_frame(DARK_GRAY, WIDTH, HEIGHT, "Frame");

    while(!glfwWindowShouldClose(frame->glfw_ctx)) {
        update_frame(frame);

        frame->header.keyboard(frame);
        // updates canvas buffers
        glfwSwapBuffers(frame->glfw_ctx);
        glfwPollEvents();
    }
    del_frame(frame);
    print_memtable();

    set_exitFlag(true);
    close_logging();
    return 0;
}
