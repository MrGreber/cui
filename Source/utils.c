#include <utils.h>
#include <log.h>
#include <memio.h>
#include <texture.h>

#include <glad.h>
#include <stdio.h>
#include <glfw3.h>
#include <immintrin.h>
#include <omp.h>


void __gl_clear_error(void) {
    while (glGetError() != GL_NO_ERROR);
}


color_t hsv_to_rgb(const f32 h, const f32 s, const f32 v) {
    const f32 c = v * s;
    const f32 x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    const f32 m = v - c;

    f32 r, g, b;
    if      (h < 60)  { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else              { r = c; g = 0; b = x; }

    color_t col;
    col.r = (u8)((r + m) * 255.0f);
    col.g = (u8)((g + m) * 255.0f);
    col.b = (u8)((b + m) * 255.0f);
    col.a = 255;
    return col;
}

void aligned_memset(u32* buffer, const u32 val, const u64 size) {
    const __m256i vc = _mm256_set1_epi32(val);

    if (size < 4096) {
        u32 i = 0;
        for (; i + 8 <= size; i += 8) _mm256_storeu_si256((__m256i*)&buffer[i], vc);
        for (; i < size; i++) buffer[i] = val;
    }
    else {
        const u64 end = (size / 8) * 8;

        #pragma omp parallel
        {
            u32 i;
            #pragma omp for
            for (i = 0; i + 8 <= size; i += 8) _mm256_storeu_si256((__m256i*)&buffer[i], vc);

            #pragma omp for
            for (i = end; i < size; i++) buffer[i] = val;
        }
    }
}

void* load_cursor(const char* path, const u16 width, const u16 height, const u16 hotx, const u16 hoty) {
    GLFWimage img = { .width = width, .height = height };
    img.pixels = (byte*)Texture(load_image)(path, (u32*)&img.width, (u32*)&img.height);
    if (!img.pixels) return NULL;

    GLFWcursor* cursor = glfwCreateCursor(&img, hotx, hoty);
    if (!cursor) return NULL;
    del_buf(&(buf_t){.ptr = (void*)img.pixels, .size = width * height * sizeof(color_t), .tag = MEMTAG_COLOR});

    return cursor;
}
