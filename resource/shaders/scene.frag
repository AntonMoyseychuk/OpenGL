#version 430 core

layout (location = 0) out vec4 frag_color;
layout (location = 1) out vec4 bright_color;

in VS_OUT {
    vec3 frag_pos;
    vec3 normal;
    vec2 texcoord;
} fs_in;

struct PointLight {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float linear;
    float quadratic;
};

struct DirectionalLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    sampler2D shadow_map;
};

struct SpotLight {
    vec3 position;
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float cutoff;

    sampler2D shadow_map;
};

struct Material {
    sampler2D diffuse0;
    sampler2D specular0;
    sampler2D emission0;

    float shininess;
};

layout (std140, binding = 2) buffer LightSpaceMatrices {
    mat4 dir_light_space_mat;
};

#define MAX_POINT_LIGTHS_COUNT 10
#define MAX_SPOT_LIGTHS_COUNT 10

uniform uint u_point_lights_count;
uniform uint u_spot_lights_count;

uniform DirectionalLight u_dir_light;
uniform PointLight u_point_lights[MAX_POINT_LIGTHS_COUNT];
uniform SpotLight u_spot_lights[MAX_SPOT_LIGTHS_COUNT];

uniform Material u_material;

uniform vec3 u_view_position;
uniform bool u_use_blinn_phong;

vec3 calculate_ambient(vec3 light_ambient, vec3 ambient_map) {
    return light_ambient * ambient_map;
}

vec3 calculate_diffuse(vec3 light_direction, vec3 normal, vec3 light_diffuse, vec3 diffuse_map) {
    const float diff = max(dot(normal, -light_direction), 0.0);
    return light_diffuse * diff * diffuse_map;
}

vec3 calculate_specular(bool use_blinn_phong_model, vec3 view_direction, vec3 light_direction, vec3 normal, vec3 light_specular, vec3 specular_map) {
    if (use_blinn_phong_model) {
        const vec3 half_direction = normalize(light_direction + view_direction);
        const float spec = pow(max(dot(normal, -half_direction), 0.0), u_material.shininess);
        return light_specular * spec * specular_map;
    } else {
        const vec3 reflect_direction = normalize(reflect(light_direction, normal));
        const float spec = pow(max(dot(-view_direction, reflect_direction), 0.0), u_material.shininess);
        return light_specular * spec * specular_map;
    }
}

float calculate_directional_shadow(vec4 frag_pos_light_space, vec3 normal, vec3 light_direction) {
    const vec3 proj_coord = (frag_pos_light_space.xyz / frag_pos_light_space.w + 1.0f) / 2.0f;

    if (proj_coord.z > 1.0f) {
        return 1.0f;
    }
    
    const float bias = max(0.05f * (1.0f - dot(normal, -light_direction)), 0.005f);
    const vec2 offset = 1.0f / textureSize(u_dir_light.shadow_map, 0);

    float shadow = 0.0f;

    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            float closest_depth = texture(u_dir_light.shadow_map, proj_coord.xy + vec2(x, y) * offset).r;
            shadow += (closest_depth + bias < proj_coord.z ? 0.0f : 1.0f);
        }
    }

    return shadow / 9.0f;
}

float calculate_spot_shadow(uint index, vec4 frag_pos_light_space, vec3 normal, vec3 light_direction) {
    const vec3 proj_coord = (frag_pos_light_space.xyz / frag_pos_light_space.w + 1.0f) / 2.0f;
    
    const float bias = max(0.005f * (1.0f - dot(normal, -light_direction)), 0.0005f);
    const vec2 offset = 1.0f / textureSize(u_spot_lights[index].shadow_map, 0);

    float shadow = 0.0f;

    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            float closest_depth = texture(u_spot_lights[index].shadow_map, proj_coord.xy + vec2(x, y) * offset).r;
            shadow += (proj_coord.z > closest_depth + bias ? 0.0f : 1.0f);
        }
    }

    return shadow / 9.0f;
}

vec3 calculate_directional_light(DirectionalLight light, vec3 normal, vec3 diffuse_map, vec3 specular_map) {
    vec3 color = calculate_ambient(light.ambient, diffuse_map);   

    const vec3 light_direction = normalize(light.direction);
    const float shadow = calculate_directional_shadow(dir_light_space_mat * vec4(fs_in.frag_pos, 1.0f), normal, light_direction);
    
    color += calculate_diffuse(light_direction, normal, light.diffuse, diffuse_map) * shadow;

    const vec3 view_direction = normalize(fs_in.frag_pos - u_view_position);
    color += calculate_specular(u_use_blinn_phong, view_direction, light_direction, normal, 
        light.specular, specular_map) * shadow;

    return color;
}

vec3 calculate_point_light(uint index, vec3 normal, vec3 diffuse_map, vec3 specular_map) {
    const float dist = length(u_point_lights[index].position - fs_in.frag_pos);
    const float attenuation = 1.0 / (1.0 + u_point_lights[index].linear * dist + u_point_lights[index].quadratic * (dist * dist));

    vec3 color = calculate_ambient(u_point_lights[index].ambient, diffuse_map) * attenuation;

    const vec3 light_direction = normalize(fs_in.frag_pos - u_point_lights[index].position);
    color += calculate_diffuse(light_direction, normal, u_point_lights[index].diffuse, diffuse_map) * attenuation;

    const vec3 view_direction = normalize(fs_in.frag_pos - u_view_position);
    color += calculate_specular(u_use_blinn_phong, view_direction, light_direction, normal, 
        u_point_lights[index].specular, specular_map) * attenuation;

    return color;
}

vec3 calculate_spot_light(uint index, vec3 normal, vec3 diffuse_map, vec3 specular_map) {
    vec3 color = calculate_ambient(u_spot_lights[index].ambient, diffuse_map);
    
    const vec3 light_direction = normalize(fs_in.frag_pos - u_spot_lights[index].position);

    const float theta = dot(light_direction, normalize(u_spot_lights[index].direction));
    if (theta > u_spot_lights[index].cutoff) {
        // const float shadow = calculate_spot_shadow(index, dir_light_space_mat * vec4(fs_in.frag_pos, 1.0f), normal, light_direction);

        color += calculate_diffuse(light_direction, normal, u_spot_lights[index].diffuse, diffuse_map);

        const vec3 view_direction = normalize(fs_in.frag_pos - u_view_position);
        color += calculate_specular(u_use_blinn_phong, view_direction, light_direction, normal, 
            u_spot_lights[index].specular, specular_map);
    }
    
    return color;
}

void main() {
    const vec4 diffuse_map = texture(u_material.diffuse0, fs_in.texcoord);
    const vec4 specular_map = texture(u_material.specular0, fs_in.texcoord);

    const vec3 normal = normalize(fs_in.normal);

    vec3 out_color = calculate_directional_light(u_dir_light, normal, diffuse_map.rgb, specular_map.rgb);

    for (uint i = 0; i < min(u_point_lights_count, MAX_POINT_LIGTHS_COUNT); ++i) {
        out_color += calculate_point_light(i, normal, diffuse_map.rgb, specular_map.rgb);
    }

    for (uint i = 0; i < min(u_spot_lights_count, MAX_SPOT_LIGTHS_COUNT); ++i) {
        out_color += calculate_spot_light(i, normal, diffuse_map.rgb, specular_map.rgb);
    }

    frag_color = vec4(out_color, diffuse_map.a);

    const float brightness = dot(frag_color.rgb, vec3(0.2126, 0.7152, 0.0722));
    bright_color = (brightness > 1.0f) ? vec4(frag_color.rgb, 1.0f) : vec4(0.0f, 0.0f, 0.0f, 1.0f);
}