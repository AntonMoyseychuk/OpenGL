#version 460 core

out vec4 frag_color;

in VS_OUT {
    vec3 normal_worldspace;
    vec2 texcoord;
} fs_in;

uniform sampler2D u_texture;


void main() {
    frag_color = texture(u_texture, fs_in.texcoord);
}