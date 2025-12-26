#include <texture.h>
#include <mem.h>
#include <utils.h>
#include <log.h>

#include <glad.h>

texture_t* new_texture(const u32 width, const u32 height) {
    buf_t buffer = {
        .size = sizeof(texture_t),
        .tag = MEMTAG_TEXTURE,
    };
    if (!new_buf(&buffer, false)) return NULL;

    texture_t* tex = buffer.ptr;

    glcall(glGenTextures(1, &tex->id), cleanup, "new_texture - Failed to generate texture.");
    glcall(glBindTexture(GL_TEXTURE_2D, tex->id), cleanup, "new_texture - Failed to bind texture");
    glcall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE), cleanup, "new_texture - Failed to set texture parameter WRAP_S.");
    glcall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE), cleanup, "new_texture - Failed to set texture parameter WRAP_T.");
    glcall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST), cleanup, "new_texture - Failed to set texture parameter MIN_FILTER.");
    glcall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST), cleanup, "new_texture - Failed to set texture parameter MAG_FILTER.");

    glcall(glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        width, height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        NULL
    ), cleanup, "new_texture - Failed to allocate texture.");

    glcall(glGenFramebuffers(1, &tex->fb_id), cleanup, "new_texture - Failed to generate frame buffer.");
    glcall(glBindFramebuffer(GL_FRAMEBUFFER, tex->fb_id), cleanup, "new_texture - Failed to bind frame buffer.");
    glcall(glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        tex->id,
        0
    ), cleanup, "new_texture - Failed to set frame buffer texture");
    glcall(glDrawBuffers(1, (u32[1]){ GL_COLOR_ATTACHMENT0 }), cleanup, "new_texture - Failed to set color attachment.");

    return tex;
cleanup:
    glDeleteFramebuffers(1, &tex->fb_id);
    glDeleteTextures(1, &tex->id);
    del_buf(&(buf_t){.size = sizeof(texture_t), .tag = MEMTAG_TEXTURE, .ptr = tex});
    return NULL;
}

void del_texture(texture_t* tex) {
    if (!tex) return;

    glDeleteFramebuffers(1, &tex->fb_id);
    glDeleteTextures(1, &tex->id);
    del_buf(&(buf_t){.size = sizeof(texture_t), .tag = MEMTAG_TEXTURE, .ptr = tex});
}

void bind_texture(const texture_t* tex) {
    if (!tex) return;
    glBindTexture(GL_TEXTURE_2D, tex->id);
}
void unbind_texture() {
    glBindTexture(GL_TEXTURE_2D, 0);
}
// ToDo: change this to load a texture and not to flush a texture to a color, overall change this to a more useful function.
void flush_texture(const texture_t* tex, const u32 width, const u32 height, const color_t bg) {
    glBindFramebuffer(GL_FRAMEBUFFER, tex->fb_id);
    //glViewport(0, 0, width, height);
    glClearColor(
        bg.r / 255.0f,
        bg.g / 255.0f,
        bg.b / 255.0f,
        bg.a / 255.0f
    );
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void set_texture_pixel(const color_t color, const i32 x, const i32 y) {
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color);
}
void draw_texture_line(const color_t color, i32 x0, i32 y0, const i32 x1, const i32 y1) {
    const i32 dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    const i32 dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    i32 err = dx + dy;

    for (;;) {
        set_texture_pixel(color, x0, y0);
        if (x0 == x1 && y0 == y1) break;
        const i32 e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
