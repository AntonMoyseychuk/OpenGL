#version 430 core

out vec3 in_texcoord;

layout (location = 0) in vec3 a_position;

layout (std140, binding = 0) uniform Matrices {
    uniform mat4 u_view;
    uniform mat4 u_projection;
};

void main() {
    in_texcoord = a_position;
    gl_Position = (u_projection * mat4(mat3(u_view)) * vec4(a_position, 1.0)).xyww;
}