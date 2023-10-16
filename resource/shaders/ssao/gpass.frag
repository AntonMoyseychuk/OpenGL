#version 460 core

layout (location = 0) out vec4 gbuff_position;
layout (location = 1) out vec4 gbuff_normal;

in VS_OUT {
    vec4 frag_pos_viewspace;
    vec4 normal_viewspace;
} fs_in;

void main() {
    gbuff_position = vec4(fs_in.frag_pos_viewspace.xyz, 1.0f);
    gbuff_normal = vec4(normalize(fs_in.normal_viewspace.xyz), 1.0f);
}