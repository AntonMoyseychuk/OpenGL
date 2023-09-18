#version 460 core
layout(location = 0) out vec4 gbuff_position;
layout(location = 1) out vec4 gbuff_normal;
layout(location = 2) out vec4 gbuff_albedo;

in VS_OUT {
    vec3 frag_pos_viewspace;
    vec3 frag_normal_viewspace;
    vec2 texcoord;

    mat3 TBN;
} fs_in;

struct Material {
    sampler2D diffuse0;
    sampler2D normal0;
    sampler2D specular0;
};
uniform Material u_material;

uniform bool u_debug_buffers = true;

uniform bool u_reverse_normals = false;

uniform bool u_use_flat_color = true;
uniform vec3 u_color = vec3(1.0);

vec3 calc_normal(vec3 normal_from_map) {
    const vec3 normal = 2.0f * normalize(normal_from_map) - 1.0f;
    return normalize(fs_in.TBN * normal);
}

void main() {
    const vec3 normal = u_use_flat_color ? 
        normalize(fs_in.frag_normal_viewspace) : 
        calc_normal(texture(u_material.normal0, fs_in.texcoord).xyz);

    gbuff_position = vec4(fs_in.frag_pos_viewspace, 1.0f);
    gbuff_normal = vec4(u_reverse_normals ? -normal : normal, u_debug_buffers ? 1.0f : 0.0f);
    gbuff_albedo.rgb = u_use_flat_color ? u_color : texture(u_material.diffuse0, fs_in.texcoord).xyz;
    gbuff_albedo.a = u_use_flat_color ? 1.0f : texture(u_material.specular0, fs_in.texcoord).r;
}