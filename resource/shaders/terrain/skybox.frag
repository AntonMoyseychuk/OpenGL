#version 460 core

out vec4 frag_color;

in VS_OUT {
    vec3 texcoord;
} fs_in;

uniform samplerCube u_skybox;

void main() {
    frag_color = texture(u_skybox, fs_in.texcoord);
}