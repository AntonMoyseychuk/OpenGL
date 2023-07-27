#version 430 core

out vec4 frag_color;

in vec2 in_texcoord;

uniform sampler2D u_texture;

void main() {
    frag_color = texture(u_texture, in_texcoord);
}