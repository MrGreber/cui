#version 330 core

in vec2 o_tpos;
out vec4 out_color;

uniform sampler2D tex;

void main() {
    out_color = texture(tex, o_tpos);
}
