#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_model, u_view, u_projection;

out VS_OUT {
    vec3 frag_pos_worldspace;
    vec4 frag_pos_clipspace;
    vec3 normal;

    float visibility;
} vs_out;

uniform struct Fog {
    vec3 color;
    
    float density;
    float gradient;
} u_fog;

void main() {
    vs_out.frag_pos_worldspace = vec3(u_model * vec4(a_position, 1.0f));
    const vec4 frag_pos_viewspace =  u_view * vec4(vs_out.frag_pos_worldspace, 1.0f);
    vs_out.frag_pos_clipspace = u_projection * frag_pos_viewspace;
    vs_out.normal = transpose(inverse(mat3(u_model))) * a_normal;

    float distance = length(frag_pos_viewspace.xyz);
    vs_out.visibility = exp(-pow(distance * u_fog.density, u_fog.gradient));
    vs_out.visibility = clamp(vs_out.visibility, 0.0f, 1.0f);

    gl_Position = vs_out.frag_pos_clipspace;
}