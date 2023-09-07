#version 430 core

out vec4 frag_color;

in vec2 texcoord;

uniform vec3 u_camera_position;

struct GBuffer {
    sampler2D position_buffer, normal_buffer, albedo_spec_buffer;
};
uniform GBuffer u_gbuffer;

struct PointLight {
    vec3 position;
    vec3 color;

    float intensity;

    float constant;
    float linear;
    float quadratic;
};

struct Material {
    float shininess;
};
uniform Material u_material;

const uint LIGHT_NUMBER = 32;
uniform PointLight u_lights[LIGHT_NUMBER];

void main() {
    const vec3 frag_pos = texture(u_gbuffer.position_buffer, texcoord).rgb;
    const vec3 normal = normalize(texture(u_gbuffer.normal_buffer, texcoord).rgb);
    const vec4 albedo_spec = texture(u_gbuffer.albedo_spec_buffer, texcoord);

    const vec3 view_direction = normalize(frag_pos - u_camera_position);

    const vec3 ambient = 0.1f * albedo_spec.rgb;

    vec3 diffuse = vec3(0.0f), specular = vec3(0.0f);
    for (uint i = 0; i < LIGHT_NUMBER; ++i) {
        const vec3 light_direction = normalize(frag_pos - u_lights[i].position);
        const float dist = length(frag_pos - u_lights[i].position);
        
        const vec3 half_direction = normalize(view_direction + light_direction);

        const float attenuation = u_lights[i].intensity / (u_lights[i].constant + u_lights[i].linear * dist + u_lights[i].quadratic * dist * dist);

        const float diff = max(dot(-light_direction, normal), 0.0f);
        diffuse += diff * albedo_spec.rgb * u_lights[i].color * attenuation;

        const float spec = pow(max(dot(normal, -half_direction), 0.0), u_material.shininess);
        specular += spec * albedo_spec.a * attenuation;
    }

    frag_color = vec4(ambient + diffuse + specular, 1.0f);
}