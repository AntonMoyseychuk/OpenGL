#version 430 core

out vec4 frag_color;

in VS_OUT {
    vec3 frag_pos;
    vec4 light_space_frag_pos;
    vec3 normal;
} fs_in;

struct DirectionalLight {
    vec3 direction;
    vec3 color;

    float intensity;

    sampler2D shadowmap;
};
uniform DirectionalLight u_light;

struct Material {
    vec3 albedo_color;
    float shininess;
};
uniform Material u_material;

uniform vec3 u_camera_position;

float calc_shadow(vec3 normal) {
    const vec3 proj_coord = 0.5f * fs_in.light_space_frag_pos.xyz / fs_in.light_space_frag_pos.w + 0.5f;
    const float depth = proj_coord.z;

    float bias = max(0.05 * (1.0 - dot(normal, normalize(u_light.direction))), 0.005);

    const float closest  = texture(u_light.shadowmap, proj_coord.xy).r;
    return (closest + bias < depth) ? 0.2f : 1.0f;
}

void main() {
    const float shadow = calc_shadow(normalize(fs_in.normal));

    const vec3 ambient = u_material.albedo_color * 0.1f;

    const vec3 light_direction = -normalize(u_light.direction);

    const float diff = max(dot(light_direction, fs_in.normal), 0.0f);
    const vec3 diffuse = diff * u_material.albedo_color * u_light.color * u_light.intensity * shadow;

    const vec3 view_direction = normalize(u_camera_position - fs_in.frag_pos);
    const vec3 half_direction = normalize(light_direction + view_direction);

    const float spec = pow(max(dot(half_direction, fs_in.normal), 0.0f), u_material.shininess);
    const vec3 specular = spec * u_material.albedo_color * u_light.color * u_light.intensity * shadow;

    frag_color = vec4(ambient + diffuse + specular, 1.0f);
}