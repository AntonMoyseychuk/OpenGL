#version 430 core

out VS_OUT {
    vec3 normal;
} vs_out;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

layout (std140, binding = 0) uniform Matrices {
    uniform mat4 u_view;
    uniform mat4 u_projection;
};
uniform mat4 u_model;
uniform mat3 u_normal_matrix;

void main() {
    gl_Position = u_view * u_model * vec4(a_position, 1.0f);
    vs_out.normal = normalize(u_normal_matrix * a_normal);
}