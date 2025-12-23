#pragma once
#include <defines.h>
#include <math-utils.h>

#include <stddef.h>

#ifndef UTILS_H
#define UTILS_H


typedef enum callback_type {
    MOUSE_CALLBACK,
    SCROLL_CALLBACK,
    KEYBOARD_CALLBACK
} callback_type;

/**
 * @typedef callback
 * @brief Function pointer type for callbacks.
 */
typedef void (*callback)(void*);

/**
 * @union color
 * @brief RGBA color representation.
 *
 * Can be accessed as individual channels or as a packed 32-bit hex value.
 */
typedef union color {
    struct {
        u8 r; /**< Red channel (0-255) */
        u8 g; /**< Green channel (0-255) */
        u8 b; /**< Blue channel (0-255) */
        u8 a; /**< Alpha channel (0-255) */
    };
    u32 hex; /**< Packed 32-bit representation (RGBA) */
} color_t;

typedef enum component_tag {
    FRAME_COMPONENT,
    CANVAS_COMPONENT,
    BUTTON_COMPONENT,
    PANEL_COMPONENT
} comp_tag;

typedef struct bounding_box {
    u32 x, y;
    u32 width, height;
} bounding_box;

typedef enum font_type {
    PHEEN, // default pixelated hebrew-english font
    ARIAL
} font_type_t;

typedef struct component_style {
    struct {
        color_t color;
    } background;
    struct {
        color_t color;
        u32 thickness;
        u32 radius;
    } border;
    struct {
        u32 left, right, top, bottom;
    } padding;
    struct {
        u8 size;
        u8 line_spacing;
        u8 line_height;
        color_t color;
        font_type_t type;
    } font;
} comp_style;

typedef struct component_style_set {
    comp_style normal;
    comp_style hover;
} comp_style_set;

typedef struct component_header {
    u8 focus;
    bounding_box box;
    color_t bg;

    callback keyboard;
    callback mouse;
    callback scroll;
    callback resize;
    void* components;
} comp_header;

typedef struct component {
    void* data;
    comp_tag tag;
} comp_t;

#define get_header(COMP) ((comp_header*)(COMP))
#define get_style(COMP, T) ((comp_style_set*)((COMP) + offsetof(T, styles)))
#define bounded(mx, my, x, y, w, h) (((mx) >= (x) && (mx) < ((x) + (w))) && ((my) >= (y) && (my) < ((y) + (h))))

/**
 * @brief Convert HSV color values to RGB.
 * @param h Hue component (0-360)
 * @param s Saturation component (0-1)
 * @param v Value component (0-1)
 * @return Corresponding RGB color
 */
color_t hsv_to_rgb(const f32 h, const f32 s, const f32 v);


/** Common color definitions */
#define TRANSP          color_cast(0x0)
#define BLACK           color_cast(0xff000000)
#define WHITE           color_cast(0xffffffff)
#define RED             color_cast(0xff0000ff)
#define GREEN           color_cast(0xff00ff00)
#define BLUE            color_cast(0xffff0000)
#define GRAY            color_cast(0xff808080)
#define LIGHT_GRAY      color_cast(0xffc0c0c0)
#define DARK_GRAY       color_cast(0xff404040)
#define YELLOW          color_cast(0xff00ffff)
#define CYAN            color_cast(0xffffff00)
#define MAGENTA         color_cast(0xffff00ff)
#define PASTEL_RED      color_cast(0xff8080ff)
#define PASTEL_GREEN    color_cast(0xff80ff80)
#define PASTEL_BLUE     color_cast(0xffff8080)
#define ORANGE          color_cast(0xff0080ff)
#define PURPLE          color_cast(0xff8000ff)
#define PINK            color_cast(0xffff80ff)

/**
 * @def color_cast
 * @brief Convenience macro to create a color_t from a 32-bit hex value.
 * @param h 32-bit RGBA color value
 */
#define color_cast(h) ((color_t){.hex = h})

/** Convert a byte (0-255) to a float (0.0-1.0) */
#define byte_to_float(b) (((f32)b) * 0.0039215686274509803921568627451f)

/** Get time delta between frames (implementation dependent) */
f32 get_deltaTime(void);

/** Clear any OpenGL errors before a call */
void __gl_clear_error(void);

/**
 * @brief Call an OpenGL function and check for errors.
 */
#define glcall(call, cleanup, msg, ...) \
do { \
    __gl_clear_error(); \
    call; \
    GLenum err = glGetError(); \
    if (err != GL_NO_ERROR) { \
        logFatal("Function call failed: %s, OpenGL error: %d.\n"msg, #call, err, ##__VA_ARGS__); \
        goto cleanup; \
    } \
} while (0)

/**
 * @brief Read a file into memory.
 */
bool read_file(const char* path, char** out, u64* size);

#endif //UTILS_H
