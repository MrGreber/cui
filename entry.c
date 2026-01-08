#include <frame.h>
#include <log.h>
#include <math-utils.h>
#include <panel.h>
#include <app.h>
#include <edit.h>
#include <button.h>

#define WIDTH 800
#define HEIGHT 800
static void __init(app_t* app) {
    frame_t* frame = new_frame(DARK_GRAY, WIDTH, HEIGHT, "Frame");

    style_group_t group = {
        .normal = {
            .init = true,
            .background = {
                .type = BG_COLOR,
                .color = WHITE
                //.image = "C:\\Users\\roygr\\CLionProjects\\stream-draw\\Resources\\heisenberg.jpg"
            },
            .border = {
                .color = BLACK,
                .thickness = 10,
                .radius = 10
            }
        }
    };
    edit_t* edit = new_edit(frame, &group, &(bounding_box){100, 100, 400, 600});
    // panel_t* panel1 = new_panel(frame, &group, &(bounding_box){100, 100, 400, 600});
    // panel_t* panel2 = new_panel(panel1, &group, &(bounding_box){0, 0, 100, 100});
    // button_t* button = new_button(frame, &group, &(bounding_box){0, 0, 50, 50});

    app->frame = frame;
    push_app_var(app, edit);
    // push_app_var(app, panel1);
    // push_app_var(app, panel2);
    // push_app_var(app, button);
}
static void __loop(app_t* app) {
    static f32 angle = 0.0;
    const frame_t* frame = app->frame;
    edit_t* edit = get_app_var(app, 0);
    // panel_t* panel1 = get_app_var(app, 0);
    // panel_t* panel2 = get_app_var(app, 1);
    // button_t* button = get_app_var(app, 2);

    const mat4 projection = m4_ortho(0.0f, (f32)frame->header.box.width, (f32)frame->header.box.height, 0.0f, -1.0f, 1.0f);
    update_frame(frame);

    bind_edit(edit);
    update_edit(edit, &projection, 0.0f);

    // angle += 0.5f;
    // if (angle > deg(PI2)) angle -= deg(PI2);
    // if (angle < 0.0f) angle += deg(PI2);
    // bind_panel(panel1);
    // update_panel(panel1, &projection, angle);
    // bind_panel(panel2);
    // update_panel(panel2, &projection, angle);
    // bind_button(button);
    // update_button(button, &projection, 0.0f);
}
static void __exit(app_t* app) {
    frame_t* frame = app->frame;
    edit_t* edit = get_app_var(app, 0);
    // panel_t* panel1 = get_app_var(app, 0);
    // panel_t* panel2 = get_app_var(app, 1);
    // button_t* button = get_app_var(app, 2);

    del_edit(edit);
    // del_panel(panel1);
    // del_panel(panel2);
    // del_button(button);
    del_frame(frame);
}

int main(void) {
    open_logging("__log__.dat", true);
    set_exitFlag(false);

    app_t* app = new_app(__init, __loop, __exit);
    start_app(app);
    exit_app(app);

    set_exitFlag(true);
    close_logging();
    return 0;
}

/*
    FILE* stream;
    fopen_s(&stream, "bt.wav", "wb");

#define DURATION 1200
#define FREQ 44100
#define write(f, s) fwrite((s), 1, sizeof(s) - 1, f)

    const u32 samples_count = DURATION * FREQ;
    const u32 file_size = samples_count * sizeof(u16) + 44;
    const u16 NumChannels = 2;
    const u16 BitsPerSample = 16;
    const u16 BlockAlign = NumChannels * BitsPerSample / 8;
    const u32 ByteRate = FREQ * BlockAlign;

    typedef struct wav_header {
        char file_type[4];
        u32 file_size;
        char file_format[4];
        char format_id[4];
        u32 block_size;
        u16 audio_format;
        u16 channels;
        u32 rate;
        u32 byte_rate;
        u16 alignment;
        u16 bit_rate;
        char data_id[4];
        u32 data_size;
    } wav_header;

    const wav_header header = {
        "RIFF",
        file_size - 8,
        "WAVE",
        "fmt ",
        16, 1,
        NumChannels,
        FREQ, ByteRate,
        BlockAlign, BitsPerSample,
        "data", samples_count * sizeof(u16)
    };
    fwrite(&header, sizeof(wav_header), 1, stream);

    const __m256 vw = _mm256_set1_ps(PI2 / FREQ);
    const __m256 vhz_l =  _mm256_set1_ps(110.0f);
    const __m256 vhz_r =  _mm256_set1_ps(116.0f);
    const __m256 v16_max = _mm256_set1_ps((f32)INT16_MAX * 0.25f);
    const __m256 v8 = _mm256_set1_ps(8.0f);
    __m256 vi = _mm256_set_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f);

    __declspec(align(32)) f32 sample_l[8] = { 0 };
    __declspec(align(32)) f32 sample_r[8] = { 0 };
    i16* buffer = malloc(2 * sizeof(i16) * samples_count);
    for (u32 i = 0; i + 8 <= samples_count; i += 8) {
        const __m256 vt = _mm256_mul_ps(vw, vi);
        const __m256 vsample_l = _mm256_mul_ps(_mm256_sin_ps(_mm256_mul_ps(vhz_l, vt)), v16_max);
        const __m256 vsample_r = _mm256_mul_ps(_mm256_sin_ps(_mm256_mul_ps(vhz_r, vt)), v16_max);

        _mm256_store_ps(sample_l, vsample_l);
        _mm256_store_ps(sample_r, vsample_r);
        for (u32 j = 0; j < 8; j++) {
            buffer[i + j] = (i16)sample_l[j];
            buffer[i + j + 1] = (i16)sample_r[j];
        }
        vi = _mm256_add_ps(vi, v8);
    }
    fwrite(buffer, sizeof(i16), 2 * samples_count, stream);
    free(buffer);

    fclose(stream);

 */