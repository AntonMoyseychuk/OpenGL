#version 430 core

out vec4 frag_color;

in vec3 in_frag_pos;
in vec3 in_normal;

uniform vec3 u_view_position;
uniform samplerCube u_skybox;

void main() {
    const vec3 I = normalize(in_frag_pos - u_view_position);
    const vec3 R = normalize(reflect(I, normalize(in_normal)));
    frag_color = texture(u_skybox, R);
}