#version 460 core

out vec4 frag_color;

in VS_OUT {
    vec3 frag_pos;
    vec3 normal;
    vec2 texcoord;
} fs_in;

uniform vec3 u_color = vec3(1.0f);

void main() {
    frag_color = vec4(u_color, 1.0f);
}