#version 430 core

out vec4 frag_color;

in vec2 in_texcoord;

uniform sampler2D u_texture;
uniform bool u_use_gamma_correction;

void main() {
    frag_color = texture(u_texture, in_texcoord);

    if (u_use_gamma_correction) {
        frag_color.rgb = pow(frag_color.rgb, vec3(1.0f / 2.2f));
    }
}