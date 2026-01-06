#pragma once

#ifndef APP_H
#define APP_H
#include <frame.h>

typedef void (*app_init_t)(struct app*);
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

app_t* new_app(const app_init_t init, const app_loop_t loop, const app_exit_t exit);
void push_app_var(app_t* app, void* var);
void* get_app_var(const app_t* app, const u16 index);
void start_app(app_t* app);
void exit_app(app_t* app);

#endif //APP_H
