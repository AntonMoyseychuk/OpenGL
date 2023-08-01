#version 430 core

out vec3 in_texcoord;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texcoord;

layout (std140) uniform Matrices {
    uniform mat4 u_view;
    uniform mat4 u_projection;
};

void main() {
    in_texcoord = a_position;
    gl_Position = (u_projection * u_view * vec4(a_position, 1.0)).xyww;
}