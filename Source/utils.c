#include <utils.h>
#include <memio.h>
#include <texture.h>

#include <glad.h>
#include <stdio.h>
#include <glfw3.h>

void __gl_clear_error(void) {
    while (glGetError() != GL_NO_ERROR);
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
