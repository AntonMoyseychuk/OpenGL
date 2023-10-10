#version 460 core

out vec4 frag_color;

in VS_OUT {
	vec4 color;
	vec2 texcoord;
} fs_in;

uniform sampler2D u_sprite;

void main() {
	frag_color = texture(u_sprite, fs_in.texcoord);
	frag_color.rgb = mix(frag_color.rgb, fs_in.color.rgb, 0.5f);
}
