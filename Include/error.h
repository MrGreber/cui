#pragma once

#ifndef ERROR_H
#define ERROR_H
#include <defines.h>

typedef enum error_level {
    __DEBUG__ = 0,
    __WARN__,
    __ERROR__,
    __FATAL__
} error_level_t;

typedef enum error_code {
    ERR_NONE = 0,
    ERR_INVALID_PARAM,
    ERR_OUT_OF_BOUNDS,
    ERR_PARSING,
    ERR_LOADING,
    ERR_GENERATION,
    ERR_HEAP_ALLOC,
    ERR_HEAP_REALLOC,
    ERR_FILE_OPEN,
    ERR_FILE_READ,
    ERR_FILE_WRITE,
    ERR_OPENGL,
    ERR_GLFW,
    ERR_SHADER_CACHE,
    ERR_MESH_CACHE,
    ERR_COMPONENT_SYSTEM,
    ERR_STRING
} error_code_t;

typedef struct error_frame {
    error_level_t level;
    error_code_t code;
    const char* function;
    const char* file;
    u32 line;
    char message[128];
} error_frame_t;

#define Error(func) __error_##func
void private(push_error)(const error_level_t level, const error_code_t code, const char* function, const char* file, const u32 line, const char* format, ...);

u8 Error(depth)(void);
error_frame_t* Error(catch)(void);
void Error(clear)(void);
void Error(dump)(void);

void Error(init)(const char* path);
void Error(close)(void);

#define logDebug(code, fmt, ...) \
    private(push_error)(__DEBUG__, code, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define logWarn(code, fmt, ...) \
    private(push_error)(__WARN__, code, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define logError(code, fmt, ...) \
    private(push_error)(__ERROR__, code, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define logFatal(code, fmt, ...) \
    private(push_error)(__FATAL__, code, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif // ERROR_H