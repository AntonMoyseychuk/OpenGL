#version 460 core

out vec4 frag_color;

in VS_OUT {
    vec3 normal_worldspace;
    float height;
} fs_in;

const vec4 min_color = vec4(0.1f, 0.22f, 0.02f, 1.0f);
const vec4 max_color = vec4(0.6f, 0.85f, 0.1f, 1.0f);

void main() {
    frag_color = mix(min_color, max_color, fs_in.height);
}