#version 460 core

layout (location = 0) in vec3 a_position;
layout (location = 2) in vec2 a_texcoord;

out VS_OUT {
	vec4 color;
	vec2 texcoord1;
	vec2 texcoord2;
	float blend;
} vs_out;

uniform mat4 u_proj_view;
uniform vec2 u_atlas_size = vec2(8);

layout(std140, binding = 0) buffer InstancesTransform {
    mat4 u_model[];
};

layout(std140, binding = 1) buffer InstancesColor {
    vec4 u_color[];
};

layout(std140, binding = 2) buffer InstancesOffset {
    vec4 u_offset[];
};

layout(std140, binding = 3) buffer InstancesBlendFactor {
    float u_blend[];
};

void main() {
	vs_out.color = u_color[gl_InstanceID];
	vs_out.texcoord1 = u_offset[gl_InstanceID].xy + a_texcoord / u_atlas_size;
	vs_out.texcoord2 = u_offset[gl_InstanceID].zw + a_texcoord / u_atlas_size;

	vs_out.blend = u_blend[gl_InstanceID];
	
	gl_Position = u_proj_view * u_model[gl_InstanceID] * vec4(a_position, 1.0f);
}