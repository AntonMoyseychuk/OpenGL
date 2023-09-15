#version 460 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texcoord;
layout (location = 3) in vec3 a_tangent;

const uint MAX_CASCADES = 8;
out VS_OUT {
    vec3 frag_pos_worldspace;
    vec4 frag_pos_clipspace;
    vec2 texcoord;
    mat3 TBN;

    vec4 frag_pos_light_clipspace[MAX_CASCADES];
} vs_out;


uniform mat4 u_model, u_view, u_projection, u_light_space[MAX_CASCADES];
uniform uint u_cascade_count;

void main() {
    vs_out.texcoord = a_texcoord;
    vs_out.frag_pos_worldspace = vec3(u_model * vec4(a_position, 1.0f));

    for (uint i = 0; i < u_cascade_count; ++i) {
        vs_out.frag_pos_light_clipspace[i] = u_light_space[i] * vec4(vs_out.frag_pos_worldspace, 1.0f);
    }
    
    const mat3 normal_matrix = transpose(inverse(mat3(u_model)));
    const vec3 N = normalize(normal_matrix * normalize(a_normal));
    vec3 T = normalize(normal_matrix * normalize(a_tangent));
    T = normalize(T - dot(T, N) * N);
    const vec3 B = cross(N, T);
    vs_out.TBN = mat3(T, B, N);
    
    vs_out.frag_pos_clipspace = u_projection * u_view * vec4(vs_out.frag_pos_worldspace, 1.0f);
    gl_Position = vs_out.frag_pos_clipspace;
}