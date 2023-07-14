#version 430 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_color;
layout (location = 2) in vec2 a_texcoord;

out vec3 in_color;
out vec2 in_texcoord;

void main() {
    gl_Position = vec4(a_position, 1.0);
    in_color = a_color;
    in_texcoord = a_texcoord;
}