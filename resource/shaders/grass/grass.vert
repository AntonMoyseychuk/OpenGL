#version 460 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

out VS_OUT {
    float blend_factor;
} vs_out;

layout(std140, binding = 0) buffer InstanceMatrices {
    mat4 u_model_matrix[];
};

uniform mat3 u_normal_matrix = mat3(1.0f);
uniform mat4 u_view_matrix, u_projection_matrix;

const vec2 grass_min = vec2(-0.0767f, 0.0f);
const vec2 grass_max = vec2(0.0810f, 2.1739f);

uniform float u_time;
uniform vec3 u_wind_direction = normalize(vec3(1.0f, 0.0f, 0.0f));
uniform float u_wind_strength = 0.1f;

vec4 quaternion_from_axis_angle(vec3 axis, float angle) {
    float half_angle = angle / 2.0f;
    return vec4(axis * sin(half_angle), cos(half_angle));
}

vec3 rotate_vector_by_quaternion(vec3 position, vec4 quaternion) {
    return position + 2.0f * cross(quaternion.xyz, cross(quaternion.xyz, position) + quaternion.w * position);
}

void main() {
    vs_out.blend_factor = (a_position.y - grass_min.y) / grass_max.y;

    const vec3 center = vec3((grass_min.x + grass_max.x) / 2.0f, (grass_min.y + grass_max.y) / 2.0f, 0.0f);
    const float angle = sin(u_time + a_position.y) * u_wind_strength;
    const vec3 axis = cross(u_wind_direction, vec3(0.0f, 1.0f, 0.0f));

    vec4 quaternion = quaternion_from_axis_angle(axis, angle);
    vec3 rotated_position = (a_position.y > grass_min.y + 0.001f ? center + rotate_vector_by_quaternion(a_position - center, quaternion) : a_position);

    gl_Position = u_projection_matrix * u_view_matrix * u_model_matrix[gl_InstanceID] * vec4(rotated_position, 1.0f);
}