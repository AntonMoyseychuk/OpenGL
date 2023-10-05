#version 460 core

layout (location = 0) in vec3 a_position;

uniform mat4 u_proj_view;
uniform mat4 u_model;

void main() {
	gl_Position = u_proj_view * u_model * vec4(a_position, 1.0f);
}