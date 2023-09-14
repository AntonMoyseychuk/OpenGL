#version 460 core

out vec4 frag_color;

#define MAX_CASCADES 8
in VS_OUT {
    vec3 frag_pos;
    float clip_space_z;
    vec4 light_space_frag_pos[MAX_CASCADES];
    vec2 texcoord;
    mat3 TBN;
} fs_in;

struct Material {
    sampler2D diffuse0;
    sampler2D normal0;

    float shininess;
};
uniform Material u_material;

struct DirectionalLight {
    vec3 direction;
    vec3 color;

    float intensity;

    sampler2D shadowmap[MAX_CASCADES];
    float cascade_end_z[MAX_CASCADES];
};
uniform DirectionalLight u_light;

uniform vec3 u_camera_position;
uniform uint u_cascade_count;

vec3 calc_normal(vec3 normal_from_map) {
    const vec3 normal = 2.0f * normal_from_map - vec3(1.0f);
    return normalize(fs_in.TBN * normal);
}

float calc_shadow(uint cascade_index, vec3 normal) {
    const vec3 proj_coord = 0.5f * fs_in.light_space_frag_pos[cascade_index].xyz / fs_in.light_space_frag_pos[cascade_index].w + 0.5f;
    const float depth = proj_coord.z;

    if (depth > 1.0f) {
        return 1.0f;
    }

    float bias = max(0.05 * (1.0 - dot(normal, normalize(u_light.direction))), 0.005);

    const vec2 texel_size = 1.0f / textureSize(u_light.shadowmap[cascade_index], 0);
    float shadow = 0.0f;
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            const float closest  = texture(u_light.shadowmap[cascade_index], proj_coord.xy + texel_size * vec2(x, y)).r;     
            shadow += (closest + bias < depth) ? 0.2f : 1.0f;        
        }    
    }
    shadow /= 9.0f;

    return shadow;
}

void main() {
    const vec3 albedo = texture(u_material.diffuse0, fs_in.texcoord).rgb;
    const vec3 normal = calc_normal(texture(u_material.normal0, fs_in.texcoord).xyz);

    const vec3 ambient = 0.1f * albedo;

    const vec3 light_direction = normalize(u_light.direction);

    float shadow = 0.0f;
    for (uint i = 0; i < u_cascade_count; ++i) {
        if (fs_in.clip_space_z <= u_light.cascade_end_z[i]) {
            shadow = calc_shadow(i, normalize(normal));
            break;
        }
    }

    const float diff = max(dot(-light_direction, normal), 0.0f);
    const vec3 diffuse = diff * albedo * u_light.color * u_light.intensity * shadow;

    const vec3 view_direction = normalize(fs_in.frag_pos - u_camera_position);
    const vec3 half_direction = normalize(view_direction + light_direction);
    const float spec = pow(max(dot(normal, -half_direction), 0.0), u_material.shininess);
    const vec3 specular = spec * albedo * u_light.color * u_light.intensity * shadow;

    frag_color = vec4(ambient + diffuse + specular, 1.0f);
}