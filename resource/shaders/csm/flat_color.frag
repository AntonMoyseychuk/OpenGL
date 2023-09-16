#version 460 core

out vec4 frag_color;

#define MAX_CASCADES 8
in VS_OUT {
    vec3 frag_pos_worldspace;
    vec4 frag_pos_clipspace;
    vec3 normal;

    vec4 frag_pos_light_clipspace[MAX_CASCADES];
} fs_in;

struct CascadedShadowmap {
    sampler2D shadowmap[MAX_CASCADES];
    float cascade_end_z[MAX_CASCADES];
};
uniform uint u_cascade_count;

struct DirectionalLight {
    vec3 direction;
    vec3 color;

    float intensity;

    CascadedShadowmap csm;
};
uniform DirectionalLight u_light;

struct Material {
    vec3 albedo_color;
    float shininess;
};
uniform Material u_material;

uniform vec3 u_camera_position;

float calc_shadow(uint cascade_index, vec3 normal) {
    const vec3 proj_coord = 0.5f * fs_in.frag_pos_light_clipspace[cascade_index].xyz / fs_in.frag_pos_light_clipspace[cascade_index].w + 0.5f;
    const float depth = proj_coord.z;

    if (depth > 1.0f) {
        return 1.0f;
    }

    float bias = max(0.005f * (1.0f - dot(normal, normalize(u_light.direction))), 0.0005f);

    const float closest  = texture(u_light.csm.shadowmap[cascade_index], proj_coord.xy).r;
    return (closest + bias < depth) ? 0.1f : 1.0f;
}

const vec4 debug_colors[] = {
    vec4(1.0f, 0.5f, 0.5f, 0.1f),
    vec4(0.5f, 1.0f, 0.5f, 0.1f),
    vec4(0.5f, 0.5f, 1.0f, 0.1f)
};

uniform bool u_cascade_debug_mode = true;

void main() {
    const vec4 albedo = vec4(u_material.albedo_color, 1.0f);
    const vec3 normal = normalize(fs_in.normal);

    float shadow = 0.0f;
    uint debug_color_index = 0;
    for (uint i = 0; i < u_cascade_count; ++i) {
        if (fs_in.frag_pos_clipspace.z <= u_light.csm.cascade_end_z[i]) {
            shadow = calc_shadow(i, normal);
            debug_color_index = i;
            break;
        }
    }

    const vec4 ambient = 0.1f * albedo;

    const vec3 light_direction = -normalize(u_light.direction);

    const float diff = max(dot(light_direction, normal), 0.0f);
    const vec4 diffuse = diff * albedo * vec4(u_light.color, 1.0f) * u_light.intensity * shadow;

    const vec3 view_direction = normalize(u_camera_position - fs_in.frag_pos_worldspace);
    const vec3 half_direction = normalize(light_direction + view_direction);

    const float spec = pow(max(dot(half_direction, normal), 0.0f), u_material.shininess);
    const vec4 specular = spec * albedo * vec4(u_light.color, 1.0f) * u_light.intensity * shadow;

    frag_color = ambient + (diffuse + specular) * (u_cascade_debug_mode ? debug_colors[debug_color_index] : vec4(1.0f));
}