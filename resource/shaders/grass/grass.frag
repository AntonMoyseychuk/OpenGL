#version 460 core

out vec4 frag_color;

in VS_OUT {
    float blend_factor;
} fs_in;

uniform vec3 u_bottom_color;
uniform vec3 u_top_color;

void main() {
    frag_color = vec4(mix(u_bottom_color, u_top_color, fs_in.blend_factor), 1.0f);
}