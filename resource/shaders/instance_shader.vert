#version 430 core 

out VS_OUT {
    vec3 frag_pos;
    vec3 normal;
    vec2 texcoord;
} vs_out;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texcoord;
layout (location = 3) in mat4 a_instance_model;

layout (std140, binding = 0) uniform Matrices {
    uniform mat4 u_view;
    uniform mat4 u_projection;
};

uniform bool u_flip_texture;

void main() {
    gl_Position = u_projection * u_view * a_instance_model * vec4(a_position, 1.0);

    vs_out.frag_pos = vec3(a_instance_model * vec4(a_position, 1.0));
    vs_out.normal = mat3(transpose(inverse(a_instance_model))) * a_normal;

    vs_out.texcoord = u_flip_texture ? vec2(a_texcoord.x, 1.0 - a_texcoord.y) : a_texcoord;
}