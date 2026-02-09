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
    PANEL_COMPONENT,
    BUTTON_COMPONENT,
    EDIT_COMPONENT,
    CANVAS_COMPONENT,
    __COMPONENT_TAG_COUNT__
} comp_tag;

typedef enum background_type {
    BG_NONE,
    BG_COLOR,
    BG_IMAGE,
    BG_GRADIENT
} bg_type_t;

typedef struct bounding_box {
    u32 x, y;
    u32 width, height;
} bounding_box;

typedef struct style {
    u8 init;

    u64 mode;
    struct {
        union {
            color_t color;
            struct texture* texture;
            const char* image;
        };
        bg_type_t type;
    } background;
    struct {
        color_t color;
        u32 thickness;
        u32 radius;
    } border;
    struct {
        u32 left, right, top, bottom;
    } padding;
} style_t;

typedef struct style_group {
    style_t normal;
    style_t hover;
} style_group_t;

typedef struct component_header {
    u8 focus;
    bounding_box box;

    callback keyboard;
    callback mouse;
    callback scroll;
    callback resize;
    void* components;
} comp_header_t;

typedef struct component {
    void* data;
    comp_tag tag;
} comp_t;

#define get_header(COMP) ((comp_header_t*)(COMP))
#define get_style(COMP, T) ((style_group_t*)((COMP) + offsetof(T, styles)))
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

__forceinline vec4 color_v4(const color_t c) {
    return (vec4){(f32)c.r / 255.f, (f32)c.g / 255.f, (f32)c.b / 255.f, (f32)c.a / 255.f};
}
__forceinline color_t v4_color(const vec4 v) {
    return (color_t){v.x * 255.f, v.y * 255.f, v.z * 255.f, v.w * 255.f};
}

__forceinline u64 __closest_pow2(u64 n) {
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;
    return n;
}

/** Convert a byte (0-255) to a float (0.0-1.0) */
#define byte_to_float(b) (((f32)b) * 0.0039215686274509803921568627451f)

/** Get time delta between frames (implementation dependent) */
f64 get_deltaTime(void);

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

#define foreach(X, ITER) for(byte* X = ITER; *X != 0; X += sizeof(*ITER))

void aligned_memset(u32* buffer, const u32 val, const u64 size);

bool gen_texture(struct texture** out, const bounding_box* box, const style_t* style);

void* load_cursor(const char* path, const u16 width, const u16 height, const u16 hotx, const u16 hoty);

#endif //UTILS_H
