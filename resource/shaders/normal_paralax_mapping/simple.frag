#version 430 core

out vec4 frag_color;

in VS_OUT {
    vec3 frag_pos;
    vec3 normal;
    vec2 texcoord;
} fs_in;

struct Material {
    sampler2D diffuse;

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

void main() {
    const vec3 albedo = texture(u_material.diffuse, fs_in.texcoord).rgb;
    
    const vec3 normal = fs_in.normal;

    const vec3 view_direction = normalize(fs_in.frag_pos - u_camera_position);

    const vec3 ambient = 0.1f * albedo;

    const vec3 light_direction = normalize(fs_in.frag_pos - u_light.position);
    const float dist = length(fs_in.frag_pos - u_light.position);
        
    const vec3 half_direction = normalize(view_direction + light_direction);

    const float attenuation = u_light.intensity / (u_light.constant + u_light.linear * dist + u_light.quadratic * dist * dist);

    const float diff = max(dot(-light_direction, normal), 0.0f);
    const vec3 diffuse = diff * albedo * u_light.color * attenuation;

    const float spec = pow(max(dot(normal, -half_direction), 0.0), u_material.shininess);
    const vec3 specular = spec * albedo * attenuation;

    frag_color = vec4(ambient + diffuse + specular, 1.0f);
}