#version 430 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texcoord;
layout (location = 3) in vec3 a_tangent;

out VS_OUT {
    vec3 frag_pos;
    vec3 normal;
    vec2 texcoord;
    mat3 TBN;
} vs_out;

uniform mat4 u_model, u_view, u_projection;

void main() {
    vs_out.frag_pos = vec3(u_model * vec4(a_position, 1.0f));
    vs_out.texcoord = a_texcoord;

    const mat3 normal_matrix = transpose(inverse(mat3(u_model)));
    const vec3 N = normalize(normal_matrix * a_normal);
    vec3 T = normalize(normal_matrix * a_tangent);
    T = normalize(T - dot(T, N) * N);
    const vec3 B = normalize(cross(N, T));
    vs_out.TBN = mat3(T, B, N);

    vs_out.normal = N;

    gl_Position = u_projection * u_view * vec4(vs_out.frag_pos, 1.0f);
}