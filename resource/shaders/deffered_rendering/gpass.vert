#version 430 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texcoord;

out VS_OUT {
    vec3 frag_pos;
    vec3 normal;
    vec2 texcoord;
} vs_out;

uniform mat4 u_model, u_view, u_projection;

void main() {
    vs_out.frag_pos = (u_model * vec4(a_position, 1.0f)).xyz;
    vs_out.normal = (transpose(inverse(u_model)) * vec4(a_normal, 0.0f)).xyz;
    vs_out.texcoord = a_texcoord;

    gl_Position = u_projection * u_view * vec4(vs_out.frag_pos, 1.0f);
}