#version 460 core

out vec4 frag_color;

const uint CASCADE_COUNT = 3;
in VS_OUT {
    vec3 frag_pos_worldspace;
    vec4 frag_pos_clipspace;
    vec3 normal;
    vec2 texcoord;

    float visibility;

    vec4 frag_pos_light_clipspace[CASCADE_COUNT];
} fs_in;

struct CascadedShadowmap {
    sampler2D shadowmap[CASCADE_COUNT];
    float cascade_end_z[CASCADE_COUNT];
};

struct DirectionalLight {
    vec3 direction;
    vec3 color;

    CascadedShadowmap csm;
};
uniform DirectionalLight u_light;

struct Material {
    sampler2D diffuse0;
};
uniform Material u_material;

uniform struct Fog {
    vec3 color;
    
    float density;
    float gradient;
} u_fog;

const vec4 debug_colors[] = {
    vec4(1.0f, 0.5f, 0.5f, 0.2f),
    vec4(0.5f, 1.0f, 0.5f, 0.2f),
    vec4(0.5f, 0.5f, 1.0f, 0.2f)
};

uniform bool u_cascade_debug_mode = false;

float calc_shadow(uint cascade_index, vec3 normal) {
    const vec3 proj_coord = 0.5f * fs_in.frag_pos_light_clipspace[cascade_index].xyz / fs_in.frag_pos_light_clipspace[cascade_index].w + 0.5f;
    const float depth = proj_coord.z;

    if (depth > 1.0f) {
        return 1.0f;
    }

    const float bias = max(0.005f * (1.0f - dot(normal, normalize(u_light.direction))), 0.0005f);

    const vec2 texel_size = 1.0f / textureSize(u_light.csm.shadowmap[cascade_index], 0);
    float shadow = 0.0f;
    for(int x = -2; x <= 2; ++x) {
        for(int y = -2; y <= 2; ++y) {
            const float closest  = texture(u_light.csm.shadowmap[cascade_index], proj_coord.xy + texel_size * vec2(x, y)).r;     
            shadow += (closest + bias < depth) ? 0.1f : 1.0f;        
        }    
    }
    shadow /= 25.0f;

    return shadow;
}

void main() {
    const vec3 normal = normalize(fs_in.normal);
    const vec3 light_direction = normalize(-u_light.direction);

    float shadow = 0.0f;
    uint debug_color_index = 0;
    for (uint i = 0; i < CASCADE_COUNT; ++i) {
        if (fs_in.frag_pos_clipspace.z <= u_light.csm.cascade_end_z[i]) {
            shadow = calc_shadow(i, normal);
            debug_color_index = i;
            break;
        }
    }

    const vec3 color = texture(u_material.diffuse0, fs_in.texcoord).rgb;

    const vec3 ambient = 0.1f * color;

    const float diff = max(dot(light_direction, normal), 0.0f) * shadow;

    frag_color = vec4(ambient + diff * u_light.color * color, 1.0f) * (u_cascade_debug_mode ? debug_colors[debug_color_index] : vec4(1.0f));
    frag_color = mix(vec4(u_fog.color, 1.0f), frag_color, fs_in.visibility);
}