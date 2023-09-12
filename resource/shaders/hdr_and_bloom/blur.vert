#version 430 core

layout (location = 0) in vec3 a_position;
layout (location = 2) in vec2 a_texcoord;

out VS_OUT {
    vec2 texcoord;
} vs_out;

void main() {
    vs_out.texcoord = a_texcoord;
    gl_Position = vec4(a_position, 1.0f);
}