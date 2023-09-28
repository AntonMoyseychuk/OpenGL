#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;


const uint CASCADE_COUNT = 3;
out VS_OUT {
    vec3 frag_pos_localspace;
    vec3 frag_pos_worldspace;
    vec4 frag_pos_clipspace;
    vec3 normal;
    vec2 texcoord;

    float visibility;

    vec4 frag_pos_light_clipspace[CASCADE_COUNT];
} vs_out;

uniform mat4 u_model, u_view, u_projection;
uniform mat4 u_light_space[CASCADE_COUNT];

uniform struct Fog {
    vec3 color;
    
    float density;
    float gradient;
} u_fog;

uniform vec4 u_water_clip_plane = vec4(0.0f, 1.0f, 0.0f, 0.0f);

void main() {
    const mat3 normal_matrix = transpose(inverse(mat3(u_model)));

    vs_out.frag_pos_localspace = a_position;
    vs_out.frag_pos_worldspace = vec3(u_model * vec4(a_position, 1.0f));
    vs_out.normal = normalize(normal_matrix * a_normal);
    vs_out.texcoord = a_texcoord;

    for (uint i = 0; i < CASCADE_COUNT; ++i) {
        vs_out.frag_pos_light_clipspace[i] = u_light_space[i] * vec4(vs_out.frag_pos_worldspace, 1.0f);
    }
    
    const vec4 frag_pos_view_space = u_view * vec4(vs_out.frag_pos_worldspace, 1.0f);

    // gl_ClipDistance[0] = dot(u_water_clip_plane, vec4(vs_out.frag_pos_localspace, 1.0f));

    float distance = length(frag_pos_view_space.xyz);
    vs_out.visibility = exp(-pow(distance * u_fog.density, u_fog.gradient));
    vs_out.visibility = clamp(vs_out.visibility, 0.0f, 1.0f);

    vs_out.frag_pos_clipspace = u_projection * frag_pos_view_space;
    gl_Position = vs_out.frag_pos_clipspace;
}