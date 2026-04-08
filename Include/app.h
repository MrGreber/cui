#pragma once

#ifndef APP_H
#define APP_H
#include <defines.h>
typedef struct frame frame_t;

typedef bool (*app_init_t)(struct app*);
typedef void (*app_loop_t)(struct app*);
typedef void (*app_exit_t)(struct app*);

typedef struct app {
    u16 count;
    u16 capacity;
    void* vars;

    frame_t* frame;
    app_init_t init;
    app_loop_t loop;
    app_exit_t exit;
} app_t;

#define App(func) __app_##func
app_t* App(new)(const app_init_t init, const app_loop_t loop, const app_exit_t exit);
void App(push)(app_t* app, void* var);
void* App(get)(const app_t* app, const u16 index);
void App(start)(app_t* app);
void App(exit)(app_t* app);

#endif //APP_H
