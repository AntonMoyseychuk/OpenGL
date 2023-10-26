#version 460 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

out VS_OUT {
    vec3 normal_worldspace;
    float height;
} vs_out;

layout(std140, binding = 0) buffer InstanceMatrices {
    mat4 u_model_matrix[];
};

uniform mat3 u_normal_matrix = mat3(1.0f);
uniform mat4 u_view_matrix, u_projection_matrix;

const float grass_min_height = 0.0f;
const float grass_max_height = 2.1739f;

uniform float u_time;
uniform vec3 u_wind_direction = normalize(vec3(1.0f, 0.0f, 0.0f));
uniform float u_wind_strength = 0.1f;

void main() {
    vs_out.normal_worldspace = normalize(u_normal_matrix * a_normal);
    vs_out.height = (a_position.y - grass_min_height) / grass_max_height;

    vec3 position = (a_position.y > grass_min_height + 0.001f ? a_position + u_wind_direction * sin(u_time * u_wind_strength) / 10.0f : a_position);

    gl_Position = u_projection_matrix * u_view_matrix * u_model_matrix[gl_InstanceID] * vec4(position, 1.0f);
}