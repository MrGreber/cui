#version 330 core

// input
layout(location = 0) in vec2 gpos; // geometry position
layout(location = 1) in vec2 i_tpos; // texture position

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

// output
out vec2 o_tpos;

void main(void) {
    gl_Position = proj * view * model * vec4(gpos, 0.0, 1.0);
    o_tpos = i_tpos;
}