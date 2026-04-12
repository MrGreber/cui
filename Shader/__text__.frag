#version 330 core
#define f32 float
struct Font {
    vec4 fg;
    vec4 bg;
};


in vec2 o_tpos;
out vec4 out_color;

uniform sampler2D tex;
uniform Font font;

void main(void) {
    vec4 texture_color = texture(tex, o_tpos);
    //out_color = texture_color;
    if (texture_color.r == 0.0 && texture_color.g == 0.0 && texture_color.b == 0.0) out_color = texture_color * font.bg;
    else out_color = texture_color * font.fg;
}
