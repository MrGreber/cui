#version 330 core

struct Border {
    float radius;
    float thickness;
    vec4 color;
};

in vec2 o_tpos;
out vec4 out_color;

uniform sampler2D tex;
uniform vec4 mask;
uniform vec2 size;
uniform Border border;

float sdRoundRect(vec2 pos, vec2 hsize, float radius) {
    vec2 q = abs(pos) - hsize + vec2(radius);
    return length(max(q, 0.0)) - radius;
}


void main(void) {
    vec2 pos = (o_tpos - 0.5) * size;
    float d_outer = sdRoundRect(pos, size * 0.5, border.radius);

    if (d_outer > 0.0) discard;

    vec2 inner_half = size * 0.5 - vec2(border.thickness);
    float inner_radius = max(border.radius - border.thickness, 0.0);

    if (inner_half.x <= 0.0 || inner_half.y <= 0.0) {
        out_color = border.color;
        return;
    }

    float d_inner = sdRoundRect(pos, inner_half, inner_radius);
    if (d_inner > 0.0) out_color = border.color;
    else out_color = texture(tex, o_tpos) * mask;
}``