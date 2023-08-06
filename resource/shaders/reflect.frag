#version 430 core

out vec4 frag_color;

in VS_OUT {
    vec3 frag_pos;
    vec3 normal;
    vec2 texcoord;
} fs_in;

uniform vec3 u_view_position;
uniform samplerCube u_skybox;

void main() {
    const vec3 I = normalize(fs_in.frag_pos - u_view_position);
    const vec3 R = normalize(reflect(I, normalize(fs_in.normal)));
    frag_color = texture(u_skybox, R);
}