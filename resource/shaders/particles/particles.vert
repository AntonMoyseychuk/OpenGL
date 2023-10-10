#version 460 core

layout (location = 0) in vec3 a_position;
layout (location = 2) in vec2 a_texcoord;

out VS_OUT {
	vec4 color;
	vec2 texcoord;
} vs_out;

uniform mat4 u_proj_view;

layout(std140, binding = 0) buffer InstancesTransform {
    mat4 u_model[];
};

layout(std140, binding = 1) buffer InstancesColor {
    vec4 u_color[];
};

void main() {
	vs_out.color = u_color[gl_InstanceID];
	vs_out.texcoord = a_texcoord;
	
	gl_Position = u_proj_view * u_model[gl_InstanceID] * vec4(a_position, 1.0f);
}