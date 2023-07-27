#version 430 core

out vec4 frag_color;

in vec3 in_texcoord;

uniform samplerCube u_skybox;

void main() {
    frag_color = texture(u_skybox, in_texcoord);
}