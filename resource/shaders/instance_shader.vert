#version 430 core 

out VS_OUT {
    vec3 frag_pos;
    vec3 normal;
    vec2 texcoord;
} vs_out;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texcoord;

layout (std140, binding = 0) uniform Matrices {
    mat4 u_view;
    mat4 u_projection;
};

layout (std140, binding = 1) buffer InstanceMatrices {
    mat4 instance_model[];
};

uniform bool u_flip_texture;

void main() {
    gl_Position = u_projection * u_view * instance_model[gl_InstanceID] * vec4(a_position, 1.0);

    vs_out.frag_pos = vec3(instance_model[gl_InstanceID] * vec4(a_position, 1.0));
    vs_out.normal = mat3(transpose(inverse(instance_model[gl_InstanceID]))) * a_normal;

    vs_out.texcoord = u_flip_texture ? vec2(a_texcoord.x, 1.0 - a_texcoord.y) : a_texcoord;
}