#version 460 core

out vec4 frag_color;

#define MAX_CASCADES 8
in VS_OUT {
    vec3 frag_pos;
    float clip_space_z;
    vec4 light_space_frag_pos[MAX_CASCADES];
    vec3 normal;
} fs_in;

struct DirectionalLight {
    vec3 direction;
    vec3 color;

    float intensity;

    sampler2D shadowmap[MAX_CASCADES];
    float cascade_end_z[MAX_CASCADES];
};
uniform DirectionalLight u_light;

struct Material {
    vec3 albedo_color;
    float shininess;
};
uniform Material u_material;

uniform vec3 u_camera_position;
uniform uint u_cascade_count;

float calc_shadow(uint cascade_index, vec3 normal) {
    const vec3 proj_coord = 0.5f * fs_in.light_space_frag_pos[cascade_index].xyz / fs_in.light_space_frag_pos[cascade_index].w + 0.5f;
    const float depth = proj_coord.z;

    if (depth > 1.0f) {
        return 1.0f;
    }

    float bias = max(0.05 * (1.0 - dot(normal, normalize(u_light.direction))), 0.005);

    const float closest  = texture(u_light.shadowmap[cascade_index], proj_coord.xy).r;
    return (closest + bias < depth) ? 0.2f : 1.0f;
}

void main() {
    float shadow = 0.0f;
    for (uint i = 0; i < u_cascade_count; ++i) {
        if (fs_in.clip_space_z <= u_light.cascade_end_z[i]) {
            shadow = calc_shadow(i, fs_in.normal);
            break;
        }
    }

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