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
    app->capacity = 8;
    app->count = 0;
    buffer = (buf_t){
        .ptr = NULL,
        .size = sizeof(void*) * app->capacity,
        .tag = MEMTAG_POINTER
    };
    if (!Buffer(new)(&buffer, true)) {
        Buffer(del)(&(buf_t){.ptr = app, .size = sizeof(app_t), .tag = MEMTAG_APP});
        return NULL;
    }
    app->vars = buffer.ptr;

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
        app->loop((void*)app);

        glfwSwapBuffers(app->frame->ctx);
        glfwPollEvents();
    }
}
static bool private(resize_var_array)(app_t* app) {
    if (app->capacity == UINT16_MAX) {
        logWarn(ERR_HEAP_REALLOC, "Failed to resize vars app, vars reached max size %d.", UINT16_MAX);
        return false;
    }

    buf_t buffer = {
        .ptr = app->vars,
        .size = sizeof(void*) * app->capacity,
        .tag = MEMTAG_POINTER
    };
    const u16 new_cap = app->capacity << 1;
    if (!Buffer(renew)(&buffer, sizeof(void*) * new_cap)) return false;
    app->capacity = new_cap;

    return true;
}
void App(push)(app_t* app, void* var) {
    if (!app) return;

    if (app->capacity <= app->count && !private(resize_var_array)(app)) {
        logError(ERR_HEAP_REALLOC, "Failed to resize app vars.");
        return;
    }
    ((u64*)app->vars)[app->count++] = (u64)var;
}
void* App(get)(const app_t* app, const u16 index) {
    if (!app) return NULL;
    if (index >= app->count) {
        logWarn(ERR_OUT_OF_BOUNDS, "Failed to get app var %d.", index);
        return NULL;
    }

    return (void*)((u64*)app->vars)[index];
}
void App(exit)(app_t* app) {
    if (!app) return;

    app->exit((void*)app);
    Buffer(del)(&(buf_t){.ptr = app->vars, .size = sizeof(void*) * app->capacity, .tag = MEMTAG_POINTER});
    Buffer(del)(&(buf_t){.ptr = app, .size = sizeof(app_t), .tag = MEMTAG_APP});
}