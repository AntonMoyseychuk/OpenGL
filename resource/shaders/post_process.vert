#version 430 core

out vec2 in_texcoord;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texcoord;

void main() {
    gl_Position = vec4(a_position, 1.0);
    in_texcoord = a_texcoord;
}