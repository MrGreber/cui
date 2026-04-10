#include <utils.h>
#include <memio.h>
#include <texture.h>

#include <glad.h>
#include <stdio.h>
#include <glfw3.h>
#include <immintrin.h>


void __gl_clear_error(void) {
    while (glGetError() != GL_NO_ERROR);
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
    Buffer(del)(&(buf_t){.ptr = (void*)img.pixels, .size = width * height * sizeof(color_t), .tag = MEMTAG_COLOR});

    return cursor;
}
