#version 430 core

layout (location = 0) in vec3 a_position;

layout (std140, binding = 2) uniform LightSpaceMatrix {
    uniform mat4 u_light_space;
};

uniform mat4 u_model;

void main() {
    gl_Position = u_light_space * u_model * vec4(a_position, 1.0);
}