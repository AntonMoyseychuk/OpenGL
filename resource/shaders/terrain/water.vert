#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_model, u_view, u_projection;

out VS_OUT {
    vec4 frag_pos_clipspace;
    vec3 normal;
} vs_out;

void main() {
    vs_out.frag_pos_clipspace = u_projection * u_view * u_model * vec4(a_position, 1.0f);
    vs_out.normal = vec3(transpose(inverse(u_model)) * vec4(a_normal, 0.0f));

    gl_Position = vs_out.frag_pos_clipspace;
}