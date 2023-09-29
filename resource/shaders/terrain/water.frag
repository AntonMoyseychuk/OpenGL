#version 460 core

out vec4 frag_pos;

in VS_OUT {
    vec4 frag_pos_clipspace;
    vec3 normal;
} fs_in;

uniform sampler2D u_reflection_map;
uniform sampler2D u_refraction_map;

void main() {
    const vec2 ndc = (fs_in.frag_pos_clipspace.xy / fs_in.frag_pos_clipspace.w) / 2.0f + 0.5f;
    const vec2 reflect_texcoord = vec2(ndc.x, -ndc.y);
    const vec2 refract_texcoord = ndc;

    const vec4 reflection_color = texture(u_reflection_map, reflect_texcoord);
    const vec4 refraction_color = texture(u_refraction_map, refract_texcoord);

    frag_pos = mix(reflection_color, refraction_color, 0.5f);
}