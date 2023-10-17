#version 460 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texcoord;

out VS_OUT {
    vec4 frag_pos_viewspace;
    vec4 normal_viewspace;
} vs_out;

uniform mat4 u_model_matrix, u_view_matrix, u_projection_matrix, u_normal_matrix;

uniform bool u_reverse_normals = false;

void main() {
    vs_out.frag_pos_viewspace = u_view_matrix * u_model_matrix * vec4(a_position, 1.0f);
    vs_out.normal_viewspace = u_view_matrix * u_normal_matrix * vec4(a_normal, 0.0f) * (u_reverse_normals ? -1 : 1);

    gl_Position = u_projection_matrix * vs_out.frag_pos_viewspace;
}