#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;
layout(location = 3) in vec3 a_tangent;

out VS_OUT {
    vec3 frag_pos;
    vec3 normal;
    vec2 texcoord;
} vs_out;

uniform mat4 u_model, u_view, u_projection;

void main() {
    const mat3 normal_matrix = transpose(inverse(mat3(u_model)));

    vs_out.frag_pos = vec3(u_model * vec4(a_position, 1.0f));
    vs_out.normal = normalize(normal_matrix * a_normal);
    vs_out.texcoord = a_texcoord;

    gl_Position = u_projection * u_view * vec4(vs_out.frag_pos, 1.0f);
}