#include <font.h>
#include <mem.h>
#include <event_system.h>
#include <math-utils.h>
#include <frame.h>
#include <log.h>

#include <stdlib.h>
#include <memory.h>
#include <glad.h>
#include <glfw3.h>

#pragma pack(push, 1)
struct fnt_info {
    byte format[3];
    u8 version;
    u8 block_id;
    u32 block_size;

    u16 font_size;
    u8 bitField;
    u8 charSet;
    u16 stretchH;
    u8 aa;
    struct {
        u8 up;
        u8 right;
        u8 down;
        u8 left;
    } padding;
    struct {
        u8 horiz;
        u8 vert;
    } spacing;
    u8 outline;
};
struct fnt_common {
    u8 block_id;
    u32 block_size;

    u16 lineHeight;
    u16 base;
    struct {
        u16 width;
        u16 height;
    } scale;
    u16 pages;
    u8 bitField;
    struct {
        u8 a;
        u8 r;
        u8 g;
        u8 b;
    } color;
};
struct fnt_pages {
    u8 block_id;
    u32 block_size;
};
struct fnt_chars {
    u8 block_id;
    u32 block_size;
};
struct fnt_char {
    u32 id;
    u16 x;
    u16 y;
    struct {
        u16 width;
        u16 height;
    } dim;
    struct {
        u16 x;
        u16 y;
    } offset;
    u16 x_advance;
    u8 page;
    u8 channel;
};
#pragma pack(pop)


#define QUAD_SIZE (6 * sizeof(vec4)) // 6 vertices size

static u16 __atlas_map(const char_t c) {
    if (c >= ' ' && c <= '~') return 2 + c - ' ';
    return 0;
}
static char_t __key_map(const char_t c, const bool is_shift) {
    static const char_t __map[] = {')', '!', '@', '#', '$', '%', '^', '&', '*', '('};

    if (is_shift) {
        if (c >= _C_'0' && c <= _C_'9') return __map[c - '0'];
        switch (c) {
            case '-': return '_';
            case '=': return '+';
            case '`': return '~';
            case ',': return '<';
            case '.': return '>';
            case '/': return '?';
            case ';': return ':';
            case '\'': return '\"';
            case '\\': return '|';
            case '[': return '{';
            case ']': return '}';
            default: return c;
        }
    }
    if (c >= _C_'A' && c <= _C_'Z') return c + 32;
    return c;
}
static void __get_font_type(font_t* font, const char* font_name) {
    if (memcmp(font_name, "VCR OSD Mono", 12) == 0) {
        font->type = VCR_OSD_MONO;
        font->kmap = __key_map;
        font->amap = __atlas_map;
    }
}
static bool __parse_fnt(font_t* font, const char* path) {
    if (!font) return false;

    FILE* stream = NULL;
    if (fopen_s(&stream, path, "rb")) return false;

    struct fnt_info info = { 0 };
    struct fnt_common common = { 0 };
    struct fnt_pages pages = { 0 };
    struct fnt_chars chars = { 0 };
    fread(&info, sizeof(struct fnt_info), 1, stream);

    u32 length = info.block_size - 14;
    byte* font_name = malloc(sizeof(byte) * (length + 1));
    if (font_name == NULL) {
        fclose(stream);
        return false;
    }
    fread(font_name, sizeof(byte), length, stream);
    font_name[length] = 0;

    fread(&common, sizeof(struct fnt_common), 1, stream);
    __get_font_type(font, (const char*)font_name);
    font->size = info.font_size;
    font->line_height = common.lineHeight;
    *(u32*)&font->padding = *(u32*)&info.padding;
    const f32 inv_atlas_w = 1.0f / (f32)common.scale.width;
    const f32 inv_atlas_h = 1.0f / (f32)common.scale.height;
    free(font_name);

    fread(&pages, sizeof(struct fnt_pages), 1, stream);
    length = pages.block_size;
    const u64 dir_size = sizeof(__DIR__"/Resources/") - 1;
    byte* pages_name = malloc(sizeof(byte) * (dir_size + length + 1));
    if (pages_name == NULL) {
        fclose(stream);
        return false;
    }
    fread((char*)(pages_name + dir_size), sizeof(byte), length, stream);
    pages_name[length + dir_size] = 0;
    memcpy(pages_name, __DIR__"/Resources/", dir_size);
    const style_t style = {
        .init = true,
        .background = {
            .type = BG_IMAGE,
            .image = (const char*)pages_name
        },
    };
    if (!gen_texture(&font->atlas, NULL, &style)) return false;
    free(pages_name);
    fread(&chars, sizeof(struct fnt_chars), 1, stream);

    font->count = chars.block_size / (u32)sizeof(struct fnt_char);
    buf_t buffer = {
        .size = sizeof(glyph_t) * font->count,
        .tag = MEMTAG_FONT
    };
    if (!new_buf(&buffer, true)) return false;
    font->glyphs = buffer.ptr;

    glyph_t* glyph = NULL;
    struct fnt_char char_ = { 0 };
    for (u32 i = 0; i < font->count; i++) {
        fread(&char_, sizeof(struct fnt_char), 1, stream);
        glyph = &font->glyphs[i];
        glyph->id = char_.id;
        glyph->x0 = ((f32)char_.x) * inv_atlas_w;
        glyph->y0 = ((f32)char_.y) * inv_atlas_h;
        glyph->x1 = ((f32)(char_.x + char_.dim.width)) * inv_atlas_w;
        glyph->y1 = ((f32)(char_.y + char_.dim.height)) * inv_atlas_h;
        *(u32*)&glyph->dim =  *(u32*)&char_.dim;
        *(u32*)&glyph->offset =  *(u32*)&char_.offset;
        glyph->x_advance = char_.x_advance;
    }

    return true;
}


font_t* new_font(const char* path) {
    buf_t buffer = {
        .size = sizeof(font_t),
        .tag = MEMTAG_FONT
    };
    if (!new_buf(&buffer, false)) return NULL;
    font_t* font = buffer.ptr;

    if (!__parse_fnt(font, path)) goto cleanup;
    font->bg = TRANSP;
    font->fg = BLACK;
    return font;
cleanup:
    del_buf(&(buf_t){.ptr = font->glyphs, .size = sizeof(glyph_t) * font->count, .tag = MEMTAG_FONT});
    del_buf(&(buf_t){.ptr = font, .size = sizeof(font_t), .tag = MEMTAG_FONT});
    return NULL;
}
void del_font(font_t* font) {
    if (!font) return;
    del_texture(font->atlas);
    del_buf(&(buf_t){.ptr = font->glyphs, .size = sizeof(glyph_t) * font->count, .tag = MEMTAG_FONT});
    del_buf(&(buf_t){.ptr = font, .size = sizeof(font_t), .tag = MEMTAG_FONT});
}
void bind_font(const font_t* font) {
    bind_texture(font->atlas);
}

void set_font(font_t* font, const char* path, const color_t fg, const color_t bg) {
    if (!font) {
        logError("set_font - Invalid parameter edit, address %p edit.\n", NULL);
        goto exit_set_font;
    }
    if (path) {
        del_font(font);
        font = new_font(path);
        if (!font) {
            logError("set_font - Failed to load font.");
            goto exit_set_font;
        }
    }

    font->bg = bg;
    font->fg = fg;
    exit_set_font:;
}