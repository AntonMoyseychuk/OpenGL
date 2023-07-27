#version 430 core 

out vec3 in_frag_pos;
out vec3 in_normal;
out vec2 in_texcoord;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texcoord;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

uniform mat4 u_transp_inv_model;

uniform bool u_flip_texture;

void main() {
    gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);

    in_frag_pos = vec3(u_model * vec4(a_position, 1.0));
    in_normal = mat3(u_transp_inv_model) * a_normal;

    in_texcoord = u_flip_texture ? vec2(a_texcoord.x, 1.0 - a_texcoord.y) : a_texcoord;
}