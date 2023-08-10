#version 430 core 

out VS_OUT {
    vec3 frag_pos;
    vec3 normal;
    vec2 texcoord;
} vs_out;

out vec4 in_frag_pos_light_space;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_texcoord;

layout (std140, binding = 0) uniform Matrices {
    uniform mat4 u_view;
    uniform mat4 u_projection;
};

layout (std140, binding = 2) uniform LightSpaceMatrix {
    uniform mat4 u_light_space;
};

uniform mat4 u_model;
uniform mat4 u_normal_matrix;

uniform bool u_flip_texture;

void main() {
    vs_out.frag_pos = vec3(u_model * vec4(a_position, 1.0f));
    vs_out.normal = mat3(u_normal_matrix) * a_normal;
    vs_out.texcoord = u_flip_texture ? vec2(a_texcoord.x, 1.0f - a_texcoord.y) : a_texcoord;
    
    in_frag_pos_light_space = u_light_space * vec4(vs_out.frag_pos, 1.0f);

    gl_Position = u_projection * u_view * vec4(vs_out.frag_pos, 1.0f);
}