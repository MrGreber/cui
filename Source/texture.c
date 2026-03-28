#include <texture.h>
#include <memio.h>
#include <utils.h>
#include <log.h>

#include <glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

texture_t* new_texture(const color_t* data, const u32 width, const u32 height) {
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
        GL_RGBA,
        width, height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        data
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
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
void flush_texture(const texture_t* tex, const color_t bg) {
    glBindFramebuffer(GL_FRAMEBUFFER, tex->fb_id);
    // glViewport(0, 0, width, height);
    glClearColor(
        byte_to_float(bg.r),
        byte_to_float(bg.g),
        byte_to_float(bg.b),
        byte_to_float(bg.a)
    );
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void set_texture_pixel(const texture_t* tex, const color_t color, const i32 x, const i32 y) {
    bind_texture(tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color);
}
void draw_texture_line(const texture_t* tex, const color_t color, i32 x0, i32 y0, const i32 x1, const i32 y1) {
    bind_texture(tex);
    const i32 dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    const i32 dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    i32 err = dx + dy;

    for (;;) {
        set_texture_pixel(tex, color, x0, y0);
        if (x0 == x1 && y0 == y1) break;
        const i32 e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

color_t* load_texture(const char* path, u32* width, u32* height) {
    const bool resize = *width != 0 && *height != 0;

    i32 channels = 0, _width, _height;
    byte* data = stbi_load(path, &_width, &_height, &channels, 4);
    if (data == NULL) {
        logError("load_texture - Failed to load image:\n\t%s", stbi_failure_reason());
        return NULL;
    }

    buf_t buffer = {
        .size = (resize ? *width * *height : _width * _height) * sizeof(color_t),
        .tag = MEMTAG_COLOR
    };
    if (!new_buf(&buffer, false)) {
        stbi_image_free(data);
        logError("load_texture - Failed to allocate buffer for texture data.");
        return NULL;
    }

    if (resize) {
        stbir_resize_uint8_srgb(
            data, _width, _height, 0,
            buffer.ptr, *width, *height, 0,
            STBIR_RGBA
        );
    }
    else {
        memcpy(buffer.ptr, data, buffer.size);
        *width = _width;
        *height = _height;
    }

    stbi_image_free(data);
    data = buffer.ptr;

    return (color_t*)data;
}
bool gen_texture(texture_t** out, const bounding_box* box, const style_t* style) {
    if (!out || !style) return false;
    switch (style->background.type) {
        case BG_COLOR: {
            if (!box) {
                logError("gen_comp_texture - Invalid parameter, box address %p.\n", NULL);
                return false;
            }
            *out = new_texture(NULL, box->width, box->height);
            if (!*out) goto cleanup;
            flush_texture(*out, style->background.color);
            break;
        }
        case BG_IMAGE: {
            u32 width = 0;
            u32 height = 0;
            if (box) {
                width = box->width;
                height = box->height;
            }

            const color_t* data = load_texture(style->background.image, &width, &height);
            if (!data) {
                logError("gen_comp_texture - Failed to load texture.");
                goto cleanup;
            }
            *out = new_texture(data, width, height);
            del_buf(&(buf_t){.ptr = (void*)data, .size = width * height * sizeof(color_t), .tag = MEMTAG_COLOR});
            if (!*out) goto cleanup;
            break;
        }
        default: return false;
    }

    return true;
cleanup:
    logError("gen_comp_texture - Failed to generate component texture.");
    return false;
}
