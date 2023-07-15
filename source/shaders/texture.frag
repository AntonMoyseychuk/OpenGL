#version 430 core

in vec2 in_texcoord;

out vec4 frag_color;

uniform float u_intensity;
uniform sampler2D u_texture0;
uniform sampler2D u_texture1;

void main() {
    frag_color = mix(texture(u_texture0, in_texcoord), texture(u_texture1, in_texcoord), 0.75) * u_intensity;
}