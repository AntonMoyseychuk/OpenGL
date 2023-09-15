#version 460 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

#define MAX_CASCADES 8
out VS_OUT {
    vec3 frag_pos_worldspace;
    vec4 frag_pos_clipspace;
    vec3 normal;

    vec4 frag_pos_light_clipspace[MAX_CASCADES];
} vs_out;

uniform mat4 u_model, u_view, u_projection, u_light_space;
uniform uint u_cascade_count;

void main() {
    vs_out.frag_pos_worldspace = vec3(u_model * vec4(a_position, 1.0f));
    vs_out.normal = transpose(inverse(mat3(u_model))) * a_normal;

    for (uint i = 0; i < u_cascade_count; ++i) {
        vs_out.frag_pos_light_clipspace[i] = u_light_space[i] * vec4(vs_out.frag_pos_worldspace, 1.0f);
    }

    vs_out.frag_pos_clipspace = u_projection * u_view * vec4(vs_out.frag_pos_worldspace, 1.0f);
    gl_Position = vs_out.frag_pos_clipspace;
}