#version 460 core

out vec4 frag_color;

in VS_OUT {
    vec2 texcoord;
} fs_in;

struct GBuffer {
    sampler2D position;
    sampler2D normal;
    sampler2D albedo;
};
uniform GBuffer u_gbuffer;

uniform sampler2D u_ssaoblur_buffer;

struct PointLight {
    vec3 position_viewspace;
    vec3 color;

    float intensity;

    float constant;
    float linear;
    float quadratic;
};
uniform PointLight u_light;

struct Material {
    float shininess;
};
uniform Material u_material;

uniform bool u_use_ssao = true;

void main() {
    const vec3 frag_pos_view_space = texture(u_gbuffer.position, fs_in.texcoord).xyz;
    const vec3 normal_view_space = normalize(texture(u_gbuffer.normal, fs_in.texcoord).xyz);
    const vec4 albedo_spec = texture(u_gbuffer.albedo, fs_in.texcoord);
    const float ambient_factor = texture(u_ssaoblur_buffer, fs_in.texcoord).r;

    const vec3 view_direction = normalize(frag_pos_view_space);

    const vec3 ambient = u_use_ssao ? 0.3f * albedo_spec.rgb * ambient_factor : 0.1f * albedo_spec.rgb;

    const vec3 light_direction = normalize(frag_pos_view_space - u_light.position_viewspace);
    const float dist = length(frag_pos_view_space - u_light.position_viewspace);
        
    const vec3 half_direction = normalize(view_direction + light_direction);

    const float attenuation = u_light.intensity / (u_light.constant + u_light.linear * dist + u_light.quadratic * dist * dist);

    const float diff = max(dot(-light_direction, normal_view_space), 0.0f);
    const vec3 diffuse = diff * albedo_spec.rgb * u_light.color;

    const float spec = pow(max(dot(normal_view_space, -half_direction), 0.0), u_material.shininess);
    const vec3 specular = spec * albedo_spec.a * u_light.color;

    frag_color = vec4(ambient + (diffuse + specular) * attenuation, 1.0f);
}