#version 460 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texcoord;

out VS_OUT {
    vec3 frag_pos_world;
    vec3 normal_world;
    vec2 texcoord;
} vs_out;

uniform mat4 u_projection_matrix;
uniform mat4 u_view_matrix;
uniform mat4 u_model_matrix;
uniform mat3 u_normal_matrix;

void main() {
    vs_out.frag_pos_world = vec3(u_model_matrix * vec4(a_position, 1.0f));
    vs_out.normal_world = u_normal_matrix * a_normal;   
    vs_out.texcoord = a_texcoord;

    gl_Position = u_projection_matrix * u_view_matrix * vec4(vs_out.frag_pos_world, 1.0);
}