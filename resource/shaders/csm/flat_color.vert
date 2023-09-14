#version 460 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

#define MAX_CASCADES 8
out VS_OUT {
    vec3 frag_pos;
    float clip_space_z;
    vec4 light_space_frag_pos[MAX_CASCADES];
    vec3 normal;
} vs_out;

uniform mat4 u_model, u_view, u_projection, u_light_space;
uniform uint u_cascade_count;

void main() {
    vs_out.frag_pos = vec3(u_model * vec4(a_position, 1.0f));
    vs_out.normal = transpose(inverse(mat3(u_model))) * a_normal;

    for (uint i = 0; i < u_cascade_count; ++i) {
        vs_out.light_space_frag_pos[i] = u_light_space[i] * vec4(vs_out.frag_pos, 1.0f);
    }

    gl_Position = u_projection * u_view * vec4(vs_out.frag_pos, 1.0f);   
    vs_out.clip_space_z = gl_Position.z; 
}