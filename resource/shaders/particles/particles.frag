#version 460 core

out vec4 frag_color;

in VS_OUT {
	vec4 color;
	vec2 texcoord;
} fs_in;

uniform sampler2D u_sprite;

void main() {
	frag_color = fs_in.color;
}
