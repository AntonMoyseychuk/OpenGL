#version 430 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

out VS_OUT {
    vec3 frag_pos;
    vec4 light_space_frag_pos;
    vec3 normal;
} vs_out;

uniform mat4 u_model, u_view, u_projection, u_light_space;

void main() {
    vs_out.frag_pos = vec3(u_model * vec4(a_position, 1.0f));
    vs_out.light_space_frag_pos = u_light_space * u_model * vec4(a_position, 1.0f);
    vs_out.normal = transpose(inverse(mat3(u_model))) * a_normal;

    gl_Position = u_projection * u_view * vec4(vs_out.frag_pos, 1.0f);    
}