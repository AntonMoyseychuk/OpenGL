#version 430 core

layout (location = 0) in vec3 a_position;

out vec3 in_texcoord;

uniform mat4 u_view;
uniform mat4 u_projection;

void main() {
    in_texcoord = a_position;
    gl_Position = (u_projection * mat4(mat3(u_view)) * vec4(a_position, 1.0)).xyww;
}