#include <app.h>
#include <memio.h>

#include <glfw3.h>

#include <log.h>


app_t* new_app(const app_init_t init, const app_loop_t loop, const app_exit_t exit) {
    buf_t buffer = {
        .size = sizeof(app_t),
        .tag = MEMTAG_APP
    };
    if (!new_buf(&buffer, false)) return NULL;

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
    if (!new_buf(&buffer, true)) {
        del_buf(&(buf_t){.ptr = app, .size = sizeof(app_t), .tag = MEMTAG_APP});
        return NULL;
    }
    app->vars = buffer.ptr;

    if (!init((void*)app)) {
        del_buf(&buffer);
        del_buf(&(buf_t){.ptr = app, .size = sizeof(app_t), .tag = MEMTAG_APP});
        return NULL;
    }
    return app;
}
void start_app(app_t* app) {
    if (!app) return;

    while (!glfwWindowShouldClose(app->frame->ctx)) {
        update_frame(app->frame);

        app->loop((void*)app);

        glfwSwapBuffers(app->frame->ctx);
        glfwPollEvents();
    }
}
static bool __resize_app_vars(app_t* app) {
    if (app->capacity == UINT16_MAX) {
        logError("__resize_app_vars - Failed to resize vars app, vars reached max size %d.", UINT16_MAX);
        return false;
    }

    buf_t buffer = {
        .ptr = app->vars,
        .size = sizeof(void*) * app->capacity,
        .tag = MEMTAG_POINTER
    };
    const u16 new_cap = app->capacity << 1;
    if (!renew_buf(&buffer, sizeof(void*) * new_cap)) return false;
    app->capacity = new_cap;

    return true;
}
void push_app_var(app_t* app, void* var) {
    if (!app) return;

    if (app->capacity <= app->count && !__resize_app_vars(app)) goto cleanup;
    ((u64*)app->vars)[app->count++] = (u64)var;
    return;
cleanup:
    logError("push_app_var - Failed to resize app vars.");
}
void* get_app_var(const app_t* app, const u16 index) {
    if (!app) return NULL;
    if (index >= app->count) {
        logError("get_app_var - Failed to get app var, index %d out of bound.", index);
        return NULL;
    }

    return (void*)((u64*)app->vars)[index];
}
void exit_app(app_t* app) {
    if (!app) return;

    app->exit((void*)app);
    del_buf(&(buf_t){.ptr = app->vars, .size = sizeof(void*) * app->capacity, .tag = MEMTAG_POINTER});
    del_buf(&(buf_t){.ptr = app, .size = sizeof(app_t), .tag = MEMTAG_APP});
    print_memtable();
}