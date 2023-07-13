#version 430 core

in vec3 in_color;
out vec4 frag_color;

uniform float u_intensity;

void main() {
    frag_color = vec4(in_color * u_intensity, 1.0);
}