#include <error.h>

#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <stdlib.h>

#define STACK_DEPTH 32
static _Thread_local error_frame_t __error_stack[STACK_DEPTH] = { 0 };
static _Thread_local u8 __stack_head = 0;
static _Thread_local u8  __stack_count = 0;
static FILE* stream = NULL;

static const char* const __error_code_strings[] = {
    [ERR_NONE]              = "None",
    [ERR_INVALID_PARAM]     = "Invalid parameter",
    [ERR_OUT_OF_BOUNDS]     = "Out of bounds",
    [ERR_PARSING]           = "Parsing failure",
    [ERR_LOADING]           = "Loading failure",
    [ERR_GENERATION]        = "Generation failure",
    [ERR_HEAP_ALLOC]        = "Heap allocation failure",
    [ERR_HEAP_REALLOC]      = "Heap reallocation failure",
    [ERR_FILE_OPEN]         = "File open failure",
    [ERR_FILE_READ]         = "File read failure",
    [ERR_FILE_WRITE]        = "File write failure",
    [ERR_OPENGL]            = "OpenGL failure",
    [ERR_GLFW]              = "GLFW failure",
    [ERR_SHADER_CACHE]      = "Shader cache failure",
    [ERR_MESH_CACHE]        = "Mesh cache failure",
    [ERR_COMPONENT_SYSTEM]  = "Component system failure",
    [ERR_STRING]            = "String failure"
};
static const char* const __error_level_strings[] = {
    [__DEBUG__] = "Debug",
    [__WARN__]  = "Warn",
    [__ERROR__] = "Error",
    [__FATAL__] = "Fatal"
};

void private(push_error)(const error_level_t level, const error_code_t code, const char* function, const char* file, const u32 line, const char* format, ...) {
    error_frame_t* error = &__error_stack[__stack_head & (STACK_DEPTH - 1)];
    va_list args;
    va_start(args, format);
    if (vsnprintf(error->message, sizeof(error->message), format, args) == -1) return;
    va_end(args);

    error->file = file;
    error->line = line;
    error->code = code;
    error->level = level;
    error->function = function;
    __stack_head = (__stack_head + 1) & (STACK_DEPTH - 1);
    if (__stack_count < STACK_DEPTH) __stack_count++;
}
error_frame_t* Error(catch)(void) {
    return &__error_stack[(__stack_head - 1 + STACK_DEPTH) & (STACK_DEPTH - 1)];
}
u8 Error(depth)(void) { return __stack_count; }

void Error(clear)(void) {
    __stack_count = 0;
    __stack_head = 0;
}
static void private(crash_handler)(int sig) {
    Error(dump)();
    fflush(stream);
    Error(close)();
    signal(sig, SIG_DFL);
    raise(sig);
}
void Error(init)(const char* path) {
    stream = fopen(path, "w");
    if (!stream) stream = stderr;
    signal(SIGSEGV, private(crash_handler));
    signal(SIGABRT, private(crash_handler));
    signal(SIGILL, private(crash_handler));
    signal(SIGFPE, private(crash_handler));
    atexit(Error(close));
}

void Error(close)(void) {
    if (stream != stderr && stream != NULL) {
        fclose(stream);
        stream = NULL;
    }

}
void Error(dump)(void) {
    FILE* out = stream ? stream : stderr;

    for (u8 i = 0; i < __stack_count; i++) {
        const error_frame_t* error = &__error_stack[(__stack_head - __stack_count + i + STACK_DEPTH) & (STACK_DEPTH - 1)];
        fprintf(out, "%s:\n\t%s\n%s\n%s:%u - %s\n\n",
            __error_level_strings[error->level],
            __error_code_strings[error->code],
            error->message,
            error->file,
            error->line,
            error->function
        );
    }
}