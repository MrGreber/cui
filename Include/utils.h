#pragma once

#ifndef UTILS_H
#define UTILS_H
#include <defines.h>
#include <math-utils.h>

typedef union color {
    struct {
        u8 r;
        u8 g;
        u8 b;
        u8 a;
    };
    u32 hex;
} color_t;

#define Color(func) __color_##func
__forceinline color_t Color(hsv_to_rgb)(const f32 h, const f32 s, const f32 v) {
    const f32 c = v * s;
    const f32 x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    const f32 m = v - c;

    f32 r, g, b;
    if (h < 60)  { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }

    color_t col;
    col.r = (u8)((r + m) * 255.0f);
    col.g = (u8)((g + m) * 255.0f);
    col.b = (u8)((b + m) * 255.0f);
    col.a = 255;
    return col;
}

/** Convert a byte (0-255) to a float (0.0-1.0) */
#define u8tof32(b) (((f32)b) * 0.0039215686274509803921568627451f) // same as dividing by 255

__forceinline vec4 Color(to_vec4)(const color_t c) {
    return (vec4){u8tof32(c.r), u8tof32(c.g), u8tof32(c.b), u8tof32(c.a)};
}
__forceinline color_t Color(vec4_to_rgb)(const vec4 v) {
    return (color_t){v.x * 255.f, v.y * 255.f, v.z * 255.f, v.w * 255.f};
}

#define color_cast(h) ((color_t)h##u)
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
        logFatal(ERR_OPENGL, "Function call failed: %s, OpenGL error: %d.\n"msg, #call, err, ##__VA_ARGS__); \
        goto cleanup; \
    } \
} while (0)

void aligned_memset(u32* buffer, const u32 val, const u64 size);

void* load_cursor(const char* path, const u16 width, const u16 height, const u16 hotx, const u16 hoty);

#endif //UTILS_H
