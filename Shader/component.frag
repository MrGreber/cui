#version 330 core
in vec2 o_tpos;
out vec4 out_color;

uniform sampler2D canvas_texture;

void main() {
    out_color = texture(canvas_texture, o_tpos);
}
