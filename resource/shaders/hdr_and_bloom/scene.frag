#version 430 core

layout (location = 0) out vec4 frag_color;
layout (location = 1) out vec4 bright_color;

in VS_OUT {
    vec3 frag_pos;
    vec3 normal;
    vec2 texcoord;
    mat3 TBN;
} fs_in;

struct Material {
    sampler2D diffuse0;
    sampler2D specular0;
    sampler2D normal0;

    float shininess;
};
uniform Material u_material;

struct PointLight {
    vec3 position;
    vec3 color;

    float intensity;
    float constant;
    float linear;
    float quadratic;
};
uniform PointLight u_light;

uniform vec3 u_camera_position;
uniform bool u_use_normal_mapping;

vec3 calc_normal(vec3 normal_from_map, mat3 TBN) {
    vec3 normal = 2.0f * normal_from_map - 1.0f;
    return normalize(TBN * normal);
}

void main() {
    const vec3 albedo_color = texture(u_material.diffuse0, fs_in.texcoord).rgb;
    const vec3 specular_color = texture(u_material.specular0, fs_in.texcoord).rgb;
    
    const vec3 normal = u_use_normal_mapping ? calc_normal(texture(u_material.normal0, fs_in.texcoord).xyz, fs_in.TBN) : normalize(fs_in.normal);

    const vec3 view_direction = normalize(fs_in.frag_pos - u_camera_position);

    const vec3 ambient = 0.1f * albedo_color;

    const vec3 light_direction = normalize(fs_in.frag_pos - u_light.position);
    const float dist = length(fs_in.frag_pos - u_light.position);
        
    const vec3 half_direction = normalize(view_direction + light_direction);

    const float attenuation = u_light.intensity / (u_light.constant + u_light.linear * dist + u_light.quadratic * dist * dist);

    const float diff = max(dot(-light_direction, normal), 0.0f);
    const vec3 diffuse = diff * albedo_color * u_light.color * attenuation;

    const float spec = pow(max(dot(normal, -half_direction), 0.0), u_material.shininess);
    const vec3 specular = spec * attenuation * specular_color;

    frag_color = vec4(ambient + diffuse + specular, 1.0f);

    bright_color = (dot(frag_color.rgb, vec3(0.2126, 0.7152, 0.0722)) > 1.0f) ? 
        frag_color : vec4(0.0f, 0.0f, 0.0f, 1.0f);
}