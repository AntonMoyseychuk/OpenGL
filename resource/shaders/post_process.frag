#version 430 core

out vec4 frag_color;

in vec2 in_texcoord;

uniform sampler2D u_scene;
uniform sampler2D u_bloom_blur;

uniform float u_exposure;

uniform bool u_use_gamma_correction;

void main() {
    const vec3 bloom_color = texture(u_bloom_blur, in_texcoord).rgb;
    const vec3 hdr_color = texture(u_scene, in_texcoord).rgb + bloom_color;
    
    vec3 mapped = vec3(1.0f) - exp(-hdr_color * u_exposure);

    if (u_use_gamma_correction) {
        const float gamma = 2.2f;

        mapped = pow(mapped, vec3(1.0f / gamma));
    }

    frag_color = vec4(mapped, 1.0f);
}