#include <app.h>
#include <memio.h>
#include <error.h>
#include <frame.h>

#include <glfw3.h>

app_t* App(new)(const app_init_t init, const app_loop_t loop, const app_exit_t exit) {
    buf_t buffer = {
        .size = sizeof(app_t),
        .tag = MEMTAG_APP
    };
    if (!Buffer(new)(&buffer, false)) return NULL;

    app_t* app = buffer.ptr;
    app->init = init;
    app->loop = loop;
    app->exit = exit;

    if (!init((void*)app)) {
        Buffer(del)(&buffer);
        Buffer(del)(&(buf_t){.ptr = app, .size = sizeof(app_t), .tag = MEMTAG_APP});
        return NULL;
    }
    return app;
}

void App(start)(app_t* app) {
    if (!app) return;

    while (!glfwWindowShouldClose(app->frame->ctx)) {
        if (app->loop) app->loop((void*)app);
        Component(update)(app->frame->header.components);
        glfwSwapBuffers(app->frame->ctx);
        glfwWaitEventsTimeout(1.0 / 240.0);
    }
}

void App(exit)(app_t* app) {
    if (!app) return;
    if (app->exit) app->exit((void*)app);
    Component(del_node)(app->frame->header.components);
    Buffer(del)(&(buf_t){.ptr = app, .size = sizeof(app_t), .tag = MEMTAG_APP});
}