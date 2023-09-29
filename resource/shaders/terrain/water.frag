#version 460 core

out vec4 frag_color;

in VS_OUT {
    vec3 frag_pos_worldspace;
    vec4 frag_pos_clipspace;
    vec2 texcoord;

    float visibility;
} fs_in;

uniform struct Fog {
    vec3 color;
    
    float density;
    float gradient;
} u_fog;

uniform struct DirectionalLight {
    vec3 color;
    vec3 direction;
} u_light;

uniform sampler2D u_reflection_map;
uniform sampler2D u_refraction_map;
uniform sampler2D u_dudv_map;
uniform sampler2D u_normal_map;

uniform float u_wave_strength = 0.01f;
uniform float u_wave_move_factor;
uniform float u_wave_shininess = 20.0f;
uniform float u_specular_strength = 0.5f;

uniform vec3 u_camera_position;

void main() {
    const vec2 ndc = (fs_in.frag_pos_clipspace.xy / fs_in.frag_pos_clipspace.w) / 2.0f + 0.5f;
    
    vec2 refract_texcoord = ndc;
    vec2 reflect_texcoord = vec2(ndc.x, -ndc.y);
    
    vec2 distorted_texcoord = texture(u_dudv_map, vec2(fs_in.texcoord.x + u_wave_move_factor, fs_in.texcoord.y)).rg * 0.1f;
    distorted_texcoord = fs_in.texcoord + vec2(distorted_texcoord.x, distorted_texcoord.y + u_wave_move_factor);
    vec2 total_distortion = (texture(u_dudv_map, distorted_texcoord).rg * 2.0f - 1.0f) * u_wave_strength;

    refract_texcoord += total_distortion;
    refract_texcoord = clamp(refract_texcoord, 0.001f, 0.999f);

    reflect_texcoord += total_distortion;
    reflect_texcoord.x = clamp(reflect_texcoord.x, 0.001f, 0.999f);
    reflect_texcoord.y = clamp(reflect_texcoord.y, -0.999f, -0.001f);

    vec4 reflection_color = texture(u_reflection_map, reflect_texcoord);
    vec4 refraction_color = texture(u_refraction_map, refract_texcoord);

    const vec3 normal = normalize(2.0f * texture(u_normal_map, distorted_texcoord).xzy - 1.0f);
    
    const vec3 view_dir = normalize(u_camera_position - fs_in.frag_pos_worldspace);
    const vec3 light_dir = normalize(-u_light.direction);
    const vec3 half_dir = normalize(view_dir + light_dir);

    const float spec = pow(max(dot(half_dir, normal), 0.0f), u_wave_shininess);
    const vec4 specular = vec4(spec * u_light.color * u_specular_strength, 1.0f);

    reflection_color += specular;
    refraction_color += specular;

    const float alpha = dot(normal, view_dir);
    frag_color = mix(refraction_color, reflection_color, 1.0f - alpha);
    frag_color = mix(vec4(u_fog.color, 1.0f), frag_color, fs_in.visibility);
}