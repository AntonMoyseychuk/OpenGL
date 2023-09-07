#version 430 core

layout (location = 0) out vec3 gbuf_position;
layout (location = 1) out vec3 gbuf_normal;
layout (location = 2) out vec4 gbuf_albedo_spec;

in VS_OUT {
    vec3 frag_pos;
    vec3 normal;
    vec2 texcoord;
} fs_in;

struct Material {
    sampler2D albedo_texture;
    sampler2D specular_texture;
};  
uniform Material u_material;

void main() {
    gbuf_position = fs_in.frag_pos;
    gbuf_normal = normalize(fs_in.normal);
    gbuf_albedo_spec.rgb = texture(u_material.albedo_texture, fs_in.texcoord).rgb;
    gbuf_albedo_spec.a = texture(u_material.specular_texture, fs_in.texcoord).r;
}