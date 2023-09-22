#version 460 core

layout (location = 0) in vec3 a_position;

layout(std140, binding = 0) buffer InstanceMatrices {
    mat4 u_model[];
};

uniform mat4 u_view, u_projection;

void main() {
    gl_Position = u_projection * u_view * u_model[gl_InstanceID] * vec4(a_position, 1.0f);    
}