#version 430 core

out vec4 frag_color;

in vec2 texcoord;

uniform sampler2D u_hdr_buffer;
uniform sampler2D u_bloom_buffer;
uniform float u_exposure = 1.0f;

void main() {
    const vec3 bloom_color = texture(u_bloom_buffer, texcoord).rgb;
    const vec3 hdr_color = texture(u_hdr_buffer, texcoord).rgb + bloom_color;

    vec3 mapped = vec3(1.0f) - exp(-hdr_color * u_exposure);

    frag_color = vec4(mapped, 1.0f);
}