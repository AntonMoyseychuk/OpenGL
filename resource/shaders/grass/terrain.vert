#version 460 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texcoord;

out VS_OUT {
    vec3 normal_worldspace;
    vec2 texcoord;
} vs_out;

uniform mat4 u_model_matrix = mat4(1.0f);
uniform mat3 u_normal_matrix = mat3(1.0f);
uniform mat4 u_view_matrix, u_projection_matrix;

void main() {
    vs_out.normal_worldspace = normalize(u_normal_matrix * a_normal);
    vs_out.texcoord = a_texcoord;

    gl_Position = u_projection_matrix * u_view_matrix * u_model_matrix * vec4(a_position, 1.0f);
}