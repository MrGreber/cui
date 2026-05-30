#include <texture.h>
#include <memio.h>
#include <utils.h>
#include <error.h>

#include <glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

texture_t* Texture(new)(const color_t* data, const u32 width, const u32 height) {
    buf_t buffer = {
        .size = sizeof(texture_t),
        .tag = MEMTAG_TEXTURE,
    };
    if (!Buffer(new)(&buffer, false)) return NULL;

    texture_t* tex = buffer.ptr;
    glcall(glGenTextures(1, &tex->id), cleanup, "Failed to generate texture.");
    glcall(glBindTexture(GL_TEXTURE_2D, tex->id), cleanup, "nFailed to bind texture");
    glcall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE), cleanup, "Failed to set texture parameter WRAP_S.");
    glcall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE), cleanup, "Failed to set texture parameter WRAP_T.");
    glcall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST), cleanup, "Failed to set texture parameter MIN_FILTER.");
    glcall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST), cleanup, "Failed to set texture parameter MAG_FILTER.");

    glcall(glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width, height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        data
    ), cleanup, "Failed to allocate texture.");

    glcall(glGenFramebuffers(1, &tex->fb_id), cleanup, "Failed to generate frame buffer.");
    glcall(glBindFramebuffer(GL_FRAMEBUFFER, tex->fb_id), cleanup, "Failed to bind frame buffer.");
    glcall(glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        tex->id,
        0
    ), cleanup, "Failed to set frame buffer texture");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return tex;
cleanup:
    glDeleteFramebuffers(1, &tex->fb_id);
    glDeleteTextures(1, &tex->id);
    Buffer(del)(&(buf_t){.size = sizeof(texture_t), .tag = MEMTAG_TEXTURE, .ptr = tex});
    return NULL;
}
void Texture(del)(texture_t* tex) {
    if (!tex) return;

    glDeleteFramebuffers(1, &tex->fb_id);
    glDeleteTextures(1, &tex->id);
    Buffer(del)(&(buf_t){.size = sizeof(texture_t), .tag = MEMTAG_TEXTURE, .ptr = tex});
}

void Texture(bind)(const texture_t* tex) {
    if (!tex) return;
    glBindTexture(GL_TEXTURE_2D, tex->id);
}
void Texture(unbind)() {
    glBindTexture(GL_TEXTURE_2D, 0);
}
void Texture(flush)(const texture_t* tex, const color_t bg) {
    glBindFramebuffer(GL_FRAMEBUFFER, tex->fb_id);
    // glViewport(0, 0, width, height);
    glClearColor(
        u8tof32(bg.r),
        u8tof32(bg.g),
        u8tof32(bg.b),
        u8tof32(bg.a)
    );
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Texture(set_pixel)(const texture_t* tex, const color_t color, const i32 x, const i32 y) {
    Texture(bind)(tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color);
}
void Texture(draw_line)(const texture_t* tex, const color_t color, i32 x0, i32 y0, const i32 x1, const i32 y1) {
    Texture(bind)(tex);

    union p64 {
        struct { i32 x, y; };
        u64 packed;
    };

    union p64 delta = {
        .x = x1 - x0,
        .y = y1 - y0
    };
    const i32 sx = ((0 < delta.x) << 1) - 1;
    const i32 sy = ((0 < delta.y) << 1) - 1;

    // some bitwise magic to preform absolute value operation on two integers
    const union p64 mask = {.packed = ((delta.packed >> 31) & 0x100000001ull) * 0xffffffffull};
    delta.x += mask.x;
    delta.y += mask.y;
    delta.packed = delta.packed ^ mask.packed;
    delta.y = -delta.y;

    i32 err = delta.x + delta.y;

    while (true) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, x0, y0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color);
        if (x0 == x1 && y0 == y1) break;
        const i32 e2 = err << 1;
        if (e2 >= delta.y) {
            err += delta.y;
            x0 += sx;
        }
        if (e2 <= delta.x) {
            err += delta.x;
            y0 += sy;
        }
    }
}

color_t* Texture(load_image)(const char* path, u32* width, u32* height) {
    const bool resize = *width != 0 && *height != 0;

    i32 channels = 0, _width, _height;
    byte* data = stbi_load(path, &_width, &_height, &channels, 4);
    if (data == NULL) {
        logError(ERR_FILE_READ, "Failed to load image:\n\t%s", stbi_failure_reason());
        return NULL;
    }

    buf_t buffer = {
        .size = (resize ? *width * *height : _width * _height) * sizeof(color_t),
        .tag = MEMTAG_COLOR
    };
    if (!Buffer(new)(&buffer, false)) {
        stbi_image_free(data);
        logError(ERR_HEAP_ALLOC, "Failed to allocate buffer for texture data.");
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

bool Texture(generate)(texture_t** out, const bounding_box* box, const style_t* style) {
    if (!out || !style) return false;
    switch (style->background.type) {
        case BG_TEST: {
            if (!box) {
                logError(ERR_INVALID_PARAM, "Address %p box.\n", NULL);
                return false;
            }
            const u64 size = (u64)box->width * (u64)box->height * sizeof(color_t);
            buf_t buffer = { .size = size, .tag = MEMTAG_COLOR };
            if (!Buffer(new)(&buffer, false)) goto cleanup;

            color_t* pixels = buffer.ptr;
            const color_t palette[2] = {
                { .r = 0xff, .g = 0xff, .b = 0xff, .a = 0xff},
                { .r = 127, .g = 127, .b = 127, .a = 0xff}
            };

#ifndef SIMD
            for (u32 y = 0; y < box->height; y++) {
                for (u32 x = 0; x < box->width; x++) {
                    const u32 idx = y * box->width + x;
                    pixels[idx].hex = palette[((x >> 6) + (y >> 6)) & 1].hex;
                }
            }
#else
            const __m256i p0 = _mm256_set1_epi32(palette[0].hex);
            const __m256i p1 = _mm256_set1_epi32(palette[1].hex);
            const __m256i vinc = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
            const __m256i v1 = _mm256_set1_epi32(1);
            const __m256i v8 = _mm256_set1_epi32(8);

            const __m256i vwidth = _mm256_set1_epi32(box->width);
            __m256i vy = vinc;
            u32 y = 0;
            for (; y + 8 <= box->height; y += 8) {
                const __m256i voffset = _mm256_mullo_epi32(vy, vwidth);
                const __m256i vy64 = _mm256_set1_epi32(y >> 6);
                __m256i vx = vinc;
                u32 x = 0;
                for (; x + 8 <= box->width; x += 8) {
                    const __m256i vx64 = _mm256_srli_epi32(vx, 6);
                    const __m256i vindices = _mm256_and_si256(_mm256_add_epi32(vx64, vy64), v1);
                    const __m256i mask = _mm256_cmpeq_epi32(vindices, _mm256_setzero_si256());
                    const __m256i vpalette = _mm256_blendv_epi8(p1, p0, mask);
                    _mm256_store_si256((__m256i*)&pixels[_mm256_extract_epi32(voffset, 0) + x], vpalette);
                    _mm256_store_si256((__m256i*)&pixels[_mm256_extract_epi32(voffset, 1) + x], vpalette);
                    _mm256_store_si256((__m256i*)&pixels[_mm256_extract_epi32(voffset, 2) + x], vpalette);
                    _mm256_store_si256((__m256i*)&pixels[_mm256_extract_epi32(voffset, 3) + x], vpalette);
                    _mm256_store_si256((__m256i*)&pixels[_mm256_extract_epi32(voffset, 4) + x], vpalette);
                    _mm256_store_si256((__m256i*)&pixels[_mm256_extract_epi32(voffset, 5) + x], vpalette);
                    _mm256_store_si256((__m256i*)&pixels[_mm256_extract_epi32(voffset, 6) + x], vpalette);
                    _mm256_store_si256((__m256i*)&pixels[_mm256_extract_epi32(voffset, 7) + x], vpalette);
                    vx = _mm256_add_epi32(vx, v8);
                }
                for (; x < box->width; x++) {
                    const u32 x64 = (x >> 6);
                    pixels[_mm256_extract_epi32(voffset, 0) + x].hex = palette[(x64 + _mm256_extract_epi32(vy64, 0)) & 1].hex;
                    pixels[_mm256_extract_epi32(voffset, 1) + x].hex = palette[(x64 + _mm256_extract_epi32(vy64, 1)) & 1].hex;
                    pixels[_mm256_extract_epi32(voffset, 2) + x].hex = palette[(x64 + _mm256_extract_epi32(vy64, 2)) & 1].hex;
                    pixels[_mm256_extract_epi32(voffset, 3) + x].hex = palette[(x64 + _mm256_extract_epi32(vy64, 3)) & 1].hex;
                    pixels[_mm256_extract_epi32(voffset, 4) + x].hex = palette[(x64 + _mm256_extract_epi32(vy64, 4)) & 1].hex;
                    pixels[_mm256_extract_epi32(voffset, 5) + x].hex = palette[(x64 + _mm256_extract_epi32(vy64, 5)) & 1].hex;
                    pixels[_mm256_extract_epi32(voffset, 6) + x].hex = palette[(x64 + _mm256_extract_epi32(vy64, 6)) & 1].hex;
                    pixels[_mm256_extract_epi32(voffset, 7) + x].hex = palette[(x64 + _mm256_extract_epi32(vy64, 7)) & 1].hex;
                }
                vy = _mm256_add_epi32(vy, v8);
            }

            for (; y < box->height; y++) {
                u32 x = 0;
                const u32 offset = y * box->width;
                const __m256i vy64 = _mm256_set1_epi32(y >> 6);
                __m256i vx = vinc;
                for (; x + 8 <= box->width; x += 8) {
                    const __m256i vx64 = _mm256_srli_epi32(vx, 6);
                    const __m256i vindices = _mm256_and_si256(_mm256_add_epi32(vx64, vy64), v1);
                    const __m256i vpalette = _mm256_i32gather_epi32((i32*)palette, vindices, sizeof(u32));
                    _mm256_store_si256((__m256i*)&pixels[offset + x], vpalette);
                    vx = _mm256_add_epi32(vx, v8);
                }
                for (; x < box->width; x++) {
                    pixels[offset + x].hex = palette[((x >> 6) + (y >> 6)) & 1].hex;
                }
            }
#endif
            *out = Texture(new)(pixels, box->width, box->height);
            Buffer(del)(&(buf_t){.ptr = (void*)pixels, .size = size, .tag = MEMTAG_COLOR});
            if (!*out) goto cleanup;
            break;
        }
        case BG_COLOR: {
            if (!box) {
                logError(ERR_INVALID_PARAM, "Address %p box.\n", NULL);
                return false;
            }
            *out = Texture(new)(NULL, box->width, box->height);
            if (!*out) goto cleanup;
            Texture(flush)(*out, style->background.color);
            break;
        }
        case BG_IMAGE: {
            u32 width = 0;
            u32 height = 0;
            if (box) {
                width = box->width;
                height = box->height;
            }

            const color_t* pixels = Texture(load_image)(style->background.image, &width, &height);
            if (!pixels) {
                logError(ERR_LOADING, "Failed to load texture.");
                goto cleanup;
            }
            *out = Texture(new)(pixels, width, height);
            Buffer(del)(&(buf_t){.ptr = (void*)pixels, .size = width * height * sizeof(color_t), .tag = MEMTAG_COLOR});
            if (!*out) goto cleanup;
            break;
        }
        case BG_LINEAR_GRADIENT: {
            if (!box) {
                logError(ERR_INVALID_PARAM, "Address %p box.\n", NULL);
                return false;
            }
            const u64 size = (u64)box->width * (u64)box->height * sizeof(color_t);
            buf_t buffer = { .size = size, .tag = MEMTAG_COLOR };
            if (!Buffer(new)(&buffer, false)) goto cleanup;
#ifndef SIMD
            const struct gradient_metadata* meta = &style->background.linear_gradient->metadata;
            const u8 n = style->count;

            vec4 palette[n];
            f32 positions[n];
            for (u8 i = 0; i < n; i++) palette[i] = Color(to_vec4)(meta->colors[i]);
            if (meta->positions) for (u8 i = 0; i < n; i++) positions[i] = meta->positions[i];
            else for (u8 i = 0; i < n; i++) positions[i] = (n > 1) ? (f32)i / (f32)(n - 1) : 0.0f;
            const f32 inv_height = 1.0f / (f32)box->height;

            color_t* pixels = buffer.ptr;
            u8 seg = 1;

            for (u32 y = 0; y < box->height; y++) {
                const f32 t = y * inv_height;

                while (seg < n - 1 && t > positions[seg]) seg++;

                const f32 seg_len = positions[seg] - positions[seg - 1];
                const f32 local_t = (seg_len > 0.0f) ? (t - positions[seg - 1]) / seg_len : 1.0f;

                const vec4* a = &palette[seg - 1];
                const vec4* b = &palette[seg];
                const color_t c = Color(vec4_to_rgb)((vec4){
                    .x = local_t * (b->x - a->x) + a->x,
                    .y = local_t * (b->y - a->y) + a->y,
                    .z = local_t * (b->z - a->z) + a->z,
                    .w = local_t * (b->w - a->w) + a->w,
                });

                for (u32 x = 0; x < box->width; x++) pixels[y * box->width + x].hex = c.hex;
            }
#else
            const struct gradient_metadata* meta = &style->background.linear_gradient->metadata;
            const u8 n = style->count;

            vec4 palette[n];
            f32 positions[n];
            for (u8 i = 0; i < n; i++) palette[i] = Color(to_vec4)(meta->colors[i]);
            if (meta->positions) for (u8 i = 0; i < n; i++) positions[i] = meta->positions[i];
            else for (u8 i = 0; i < n; i++) positions[i] = (n > 1) ? (f32)i / (f32)(n - 1) : 0.0f;
            const f32 inv_height = 1.0f / (f32)box->height;
            color_t* pixels = buffer.ptr;
            u8 seg = 1;
            for (u32 y = 0; y < box->height; y++) {
                const f32 t = y * inv_height;

                while (seg < n - 1 && t > positions[seg]) seg++;
                const f32 seg_len = positions[seg] - positions[seg - 1];
                const f32 local_t = (seg_len > 0.0f) ? (t - positions[seg - 1]) / seg_len : 1.0f;

                const vec4* a = &palette[seg - 1];
                const vec4* b = &palette[seg];

                const __m128 vt = _mm_set1_ps(local_t);
                const vec4 vgrad = {.v = _mm_fmadd_ps(vt, _mm_sub_ps(b->v, a->v), a->v)};
                const color_t color = Color(vec4_to_rgb)(vgrad);
                const __m256i vcolor = _mm256_set1_epi32(color.hex);
                u32 x = 0;
                for (; x + 8 <= box->width; x += 8) _mm256_store_si256((__m256i*)&pixels[y * box->width + x], vcolor);
                for (; x < box->width; x++) pixels[y * box->width + x].hex = color.hex;
            }
#endif
            *out = Texture(new)(pixels, box->width, box->height);
            Buffer(del)(&(buf_t){.ptr = (void*)pixels, .size = size, .tag = MEMTAG_COLOR});
            if (!*out) goto cleanup;
            break;
        }
        case BG_RADIAL_GRADIENT: {
            break;
        }
        case BG_MANDELBROT: {
            if (!box) {
                logError(ERR_INVALID_PARAM, "Address %p box.\n", NULL);
                return false;
            }
            const u64 size = (u64)box->width * (u64)box->height * sizeof(color_t);
            buf_t buffer = { .size = size, .tag = MEMTAG_COLOR };
            if (!Buffer(new)(&buffer, false)) goto cleanup;
            color_t* pixels = buffer.ptr;
#ifndef SIMD
            const f32 scale = 2.5f / (f32)box->width;
            const f32 y_offset = scale * (f32)box->height * 0.5f;
            for (u32 y = 0; y < box->height; y++) {
                const f32 im = (f32)y * scale - y_offset;
                for (u32 x = 0; x < box->width; x++) {
                    const f32 re = (f32)x * scale - 2.0f;
                    const vec2 c = {re, im};
                    vec2 z = { 0 };
                    bool bounded_flag = true;

                    for (u8 i = 0; i < 255; i++) {
                        f32 xy = z.x * z.y;
                        z.x = z.x * z.x - z.y * z.y + c.x;
                        z.y = xy + xy + c.y;
                        if (z.x * z.x + z.y * z.y > 4.0f) {
                            pixels[y * box->width + x] = (color_t){i, i, i, 255};
                            bounded_flag = false;
                            break;
                        }
                    }
                    if (bounded_flag) pixels[y * box->width + x] = BLACK;
                }
            }
#else
            const f32 scale = 2.5f / (f32)box->width;
            const f32 y_offset = scale * (f32)box->height * 0.5f;

            const __m256i v01 = _mm256_set1_epi32(0x01010101);
            const __m256i valpha = _mm256_set1_epi32(0xff000000);
            const __m256 v0 = _mm256_set1_ps(0.0f);
            const __m256 v2 = _mm256_set1_ps(2.0f);
            const __m256 v4 = _mm256_set1_ps(4.0f);
            const __m256 v8 = _mm256_set1_ps(8.0f);
            const __m256 vscale = _mm256_set1_ps(scale);
            const __m256 vinc = _mm256_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f);
            for (u32 y = 0; y < box->height; y++) {
                u32 x = 0;
                const f32 c_im = (f32)y * scale - y_offset;
                const u32 offset = y * box->width;
                __m256 vx = vinc;
                const __m256 c_vim = _mm256_set1_ps(c_im);
                for (; x + 8 < box->width; x += 8) {
                    const __m256 c_vre = _mm256_fmsub_ps(vscale, vx, v2);
                    __m256 vre = v0;
                    __m256 vim = v0;

                    __m256 vbounded = v0;
                    __m256 vp = v0;
                    for (u16 i = 0; i < 256; i++) {
                        const __m256 vri = _mm256_mul_ps(vre, vim);
                        vre = _mm256_fmsub_ps(vre, vre, _mm256_fmsub_ps(vim, vim, c_vre));
                        vim = _mm256_add_ps(_mm256_add_ps(vri, vri), c_vim);
                        const __m256 vmag2 = _mm256_fmadd_ps(vre, vre, _mm256_mul_ps(vim, vim));
                        const __m256 vcmp = _mm256_cmp_ps(vmag2, v4, _CMP_GT_OQ);
                        const __m256 viter = _mm256_set1_ps((f32)i);

                        __m256 vmask = _mm256_cmp_ps(vbounded, v0, _CMP_EQ_OQ);
                        vmask = _mm256_and_ps(vmask, vcmp);
                        vp = _mm256_or_ps(vp, _mm256_and_ps(vmask, viter));

                        vbounded = _mm256_or_ps(vbounded, vcmp);
                        if (_mm256_movemask_ps(vbounded) == 0xFF) break;
                    }
                    __m256i vpalette = _mm256_cvtps_epi32(vp);
                    vpalette = _mm256_mullo_epi32(vpalette, v01);
                    vpalette = _mm256_or_si256(vpalette, valpha);
                    _mm256_store_si256((__m256i*)&pixels[offset + x], vpalette);
                    vx = _mm256_add_ps(vx, v8);
                }

                for (; x < box->width; x++) {
                    const f32 c_re = (f32)x * scale - 2.0f;
                    f32 re = 0.0f;
                    f32 im = 0.0f;
                    bool bounded_flag = true;
                    for (u16 i = 0; i < 256; i++) {
                        f32 xy = re * im;
                        re = re * re - im * im + c_re;
                        im = xy + xy + c_im;
                        if (re * re + im * im > 4.0f) {
                            pixels[offset + x] = (color_t){i, i, i, 255};
                            bounded_flag = false;
                            break;
                        }
                    }
                    if (bounded_flag) pixels[offset + x] = BLACK;
                }
            }
#endif

            *out = Texture(new)(pixels, box->width, box->height);
            Buffer(del)(&(buf_t){.ptr = (void*)pixels, .size = size, .tag = MEMTAG_COLOR});
            if (!*out) goto cleanup;
            break;
        }
        default: return false;
    }

    return true;
cleanup:
    logError(ERR_GENERATION, "Failed to generate texture of type %d.", style->background.type);
    return false;
}
