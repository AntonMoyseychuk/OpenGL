#version 430

out vec4 frag_color;

uniform vec3 u_border_color;

void main() {
    frag_color = vec4(u_border_color, 1.0);
}