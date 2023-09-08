#version 430 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texcoord;
layout (location = 3) in vec3 a_tangent;

out VS_OUT {
    vec3 frag_pos;
    vec2 texcoord;
    mat3 TBN;
} vs_out;

uniform mat4 u_model, u_view, u_projection;

void main() {
    vs_out.texcoord = a_texcoord;
    vs_out.frag_pos = vec3(u_model * vec4(a_position, 1.0f));
    
    const mat4 normal_matrix = transpose(inverse(u_model));
    const vec3 N = normalize(vec3(normal_matrix * vec4(normalize(a_normal), 0.0f)));
    vec3 T = normalize(vec3(normal_matrix * vec4(normalize(a_tangent), 0.0f)));
    T = normalize(T - dot(T, N) * N);
    const vec3 B = cross(N, T);
    vs_out.TBN = mat3(T, B, N);
    
    gl_Position = u_projection * u_view * vec4(vs_out.frag_pos, 1.0f);
}