#include <utils.h>
#include <log.h>
#include <mem.h>

#include <glad.h>
#include <stdio.h>
#include <glfw3.h>


void __gl_clear_error(void) {
    while (glGetError() != GL_NO_ERROR);
}


f32 get_deltaTime(void) {
    static f32 last = 0.0f;
    f32 current = (f32)glfwGetTime();
    f32 dt = current - last;
    last = current;
    return dt;
}


bool read_file(const char* path, char** out, u64* size) {
    FILE* stream = NULL;

    if (fopen_s(&stream, path, "rb") != 0) {
        logError("read - Failed to open file: %s.", path);
        return false;
    }

    _fseeki64(stream, 0, SEEK_END);
    const i64 pos = _ftelli64(stream);
    if (pos == -1) goto cleanup;
    _fseeki64(stream, 0, SEEK_SET);

    buf_t buffer = {
        .size = pos,
        .tag = MEMTAG_BYTE
    };
    if (!new_buf(&buffer, true)) goto cleanup;
    if (fread_s(buffer.ptr, pos, 1, pos, stream) != pos) goto cleanup;

    *out = buffer.ptr;
    *size = pos;

    fclose(stream);
    return true;
cleanup:
    if (stream) fclose(stream);
    if (buffer.ptr) del_buf(&buffer);
    return false;
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