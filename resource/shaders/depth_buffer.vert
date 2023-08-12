#version 430 core

layout (location = 0) in vec3 a_position;

layout (std140, binding = 2) buffer LightSpaceMatrices {
    mat4 dir_light_space_mat;
};

uniform mat4 u_model;

void main() {
    gl_Position = dir_light_space_mat * u_model * vec4(a_position, 1.0);
}