#version 460 core

out vec4 frag_pos;

in VS_OUT {
    vec4 frag_pos_clipspace;
    vec3 normal;
} fs_in;

uniform sampler2D u_reflection_map;
uniform sampler2D u_refraction_map;
uniform sampler2D u_dudv_map;

uniform float u_wave_strength = 0.01f;
uniform float u_move_factor;

void main() {
    const vec2 ndc = (fs_in.frag_pos_clipspace.xy / fs_in.frag_pos_clipspace.w) / 2.0f + 0.5f;
    vec2 distortion = (texture(u_dudv_map, vec2(ndc.x + u_move_factor, ndc.y)).rg * 2.0f - 1.0f) * u_wave_strength;
    
    vec2 reflect_texcoord = vec2(ndc.x, -ndc.y) + distortion;
    vec2 refract_texcoord = ndc + distortion;

    refract_texcoord = clamp(refract_texcoord, 0.001f, 0.999f);
    reflect_texcoord.x = clamp(reflect_texcoord.x, 0.001f, 0.999f);
    reflect_texcoord.y = clamp(reflect_texcoord.y, -0.999f, -0.001f);

    const vec4 reflection_color = texture(u_reflection_map, reflect_texcoord);
    const vec4 refraction_color = texture(u_refraction_map, refract_texcoord);

    frag_pos = mix(reflection_color, refraction_color, 0.5f);
}