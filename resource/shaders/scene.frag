#version 430 core

out vec4 frag_color;

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
};

struct SpotLight {
    vec3 position;
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float cutoff;
};

struct Material {
    sampler2D diffuse0;
    sampler2D specular0;
    sampler2D emission0;

    float shininess;
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

vec3 calculate_directional_light(DirectionalLight light, vec3 normal, vec3 diffuse_map, vec3 specular_map) {
    const vec3 ambient = light.ambient * diffuse_map;   

    const vec3 light_dir = normalize(light.direction);
    const float diff = max(dot(normal, -light_dir), 0.0);
    const vec3 diffuse = light.diffuse * diff * diffuse_map;

    const vec3 view_direction = normalize(fs_in.frag_pos - u_view_position);
    const vec3 half_direction = normalize(light_dir + view_direction);
    const float spec = pow(max(dot(normal, -half_direction), 0.0), u_material.shininess);
    const vec3 specular = light.specular * spec * specular_map;

    return (ambient + diffuse + specular);
}

vec3 calculate_point_light(PointLight light, vec3 normal, vec3 diffuse_map, vec3 specular_map) {
    const float dist = length(light.position - fs_in.frag_pos);
    const float attenuation = 1.0 / (1.0 + light.linear * dist + light.quadratic * (dist * dist));

    const vec3 ambient = light.ambient * diffuse_map * attenuation;

    const vec3 light_dir = normalize(fs_in.frag_pos - light.position);
    const float diff = max(dot(normal, -light_dir), 0.0);
    const vec3 diffuse = light.diffuse * diff * diffuse_map * attenuation;

    const vec3 view_direction = normalize(fs_in.frag_pos - u_view_position);
    const vec3 half_direction = normalize(light_dir + view_direction);
    const float spec = pow(max(dot(normal, -half_direction), 0.0), u_material.shininess);
    const vec3 specular = light.specular * spec * specular_map * attenuation;

    return (ambient + diffuse + specular);
}

vec3 calculate_spot_light(SpotLight light, vec3 normal, vec3 diffuse_map, vec3 specular_map) {
    const vec3 ambient = light.ambient * diffuse_map;
    
    const vec3 light_dir = normalize(fs_in.frag_pos - light.position);

    const float theta = dot(light_dir, normalize(light.direction));
    if (theta > light.cutoff) {
        const float diff = max(dot(normal, -light_dir), 0.0);
        const vec3 diffuse = light.diffuse * diff * diffuse_map;

        const vec3 view_direction = normalize(fs_in.frag_pos - u_view_position);
        const vec3 half_direction = normalize(light_dir + view_direction);
        const float spec = pow(max(dot(normal, -half_direction), 0.0), u_material.shininess);
        const vec3 specular = light.specular * spec * specular_map;

        return (ambient + diffuse + specular);
    } else {
        return ambient;
    }
}

uniform bool u_is_sphere;

void main() {
    const vec4 diffuse_map = u_is_sphere ? vec4(1.0f, 0.0f, 0.0f, 1.0f) : texture(u_material.diffuse0, fs_in.texcoord);
    const vec4 specular_map = u_is_sphere ? vec4(1.0f) : texture(u_material.specular0, fs_in.texcoord);

    const vec3 normal = normalize(fs_in.normal);

    vec3 out_color = calculate_directional_light(u_dir_light, normal, diffuse_map.rgb, specular_map.rgb);

    for (uint i = 0; i < u_point_lights_count; ++i) {
        out_color += calculate_point_light(u_point_lights[i], normal, diffuse_map.rgb, specular_map.rgb);
    }

    for (uint i = 0; i < u_spot_lights_count; ++i) {
        out_color += calculate_spot_light(u_spot_lights[i], normal, diffuse_map.rgb, specular_map.rgb);
    }

    frag_color = vec4(out_color, diffuse_map.a);
}