#version 460 core

out vec4 frag_color;

in VS_OUT {
	vec4 color;
	vec2 texcoord1;
	vec2 texcoord2;
	float blend;
} fs_in;

uniform sampler2D u_sprite;

void main() {
	const vec4 color1 = texture(u_sprite, fs_in.texcoord1);
	const vec4 color2 = texture(u_sprite, fs_in.texcoord2);
	frag_color = mix(color1, color2, fs_in.blend);
	// frag_color.rgb = mix(frag_color.rgb, fs_in.color.rgb, 0.5f);
}
