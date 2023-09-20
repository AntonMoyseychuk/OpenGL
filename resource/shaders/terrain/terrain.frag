#version 460 core

out vec4 frag_color;

in VS_OUT {
    vec3 frag_pos;
    vec3 normal;
    vec2 texcoord;
} fs_in;

struct DirectionalLight {
    vec3 direction;
    vec3 color;
};
uniform DirectionalLight u_light;

uniform vec3 u_camera_position;

struct Material {
    vec3 color;
    float shininess;
};
uniform Material u_material;

void main() {
    const vec3 normal = normalize(fs_in.normal);
    const vec3 light_direction = normalize(-u_light.direction);
    const vec3 view_direction = normalize(u_camera_position - fs_in.frag_pos);
    const vec3 half_direction = normalize(light_direction + view_direction);

    const vec3 ambient = 0.1f * u_material.color;

    const float diff = max(dot(light_direction, normal), 0.0f);
    const float spec = pow(max(dot(half_direction, normal), 0.0f), u_material.shininess);

    frag_color = vec4(ambient + (diff + spec) * u_light.color * u_material.color, 1.0f);
}