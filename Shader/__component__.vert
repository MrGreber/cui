#version 330 core

// input
layout(location = 0) in vec2 gpos; // geometry position
layout(location = 1) in vec2 i_tpos; // texture position

uniform mat4 projection;
uniform mat4 scale;
uniform mat4 rotation;
uniform mat4 position;
uniform mat4 size;
uniform mat4 inv_size;
uniform mat4 model;

// output
out vec2 o_tpos;

void main(void) {
    gl_Position =  projection * model/*position * size * rotation * inv_size * scale*/ * vec4(gpos, 0.0, 1.0);
    o_tpos = i_tpos;
}