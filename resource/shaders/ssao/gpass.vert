#version 460 core
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texcoord;
layout (location = 3) in vec3 a_tangent;

out VS_OUT {
    vec3 frag_pos_viewspace;
    vec3 frag_normal_viewspace;
    vec2 texcoord;

    mat3 TBN;
} vs_out;

uniform mat4 u_model, u_view, u_projection;

void main() {
    const mat4 VM = u_view * u_model;
    const mat3 normal_matrix = transpose(inverse(mat3(VM)));

    vs_out.frag_pos_viewspace = vec3(VM * vec4(a_position, 1.0f));
    vs_out.frag_normal_viewspace = vec3(normal_matrix * normalize(a_normal));
    vs_out.texcoord = a_texcoord;

    const vec3 N = normalize(normal_matrix * a_normal);
    vec3 T = normalize(normal_matrix * a_tangent);
    T = normalize(T - N * dot(N, T));
    const vec3 B = cross(N, T);
    vs_out.TBN = mat3(T, B, N);

    gl_Position = u_projection * vec4(vs_out.frag_pos_viewspace, 1.0f);
}