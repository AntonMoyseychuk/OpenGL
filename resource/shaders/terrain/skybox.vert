#version 460 core

layout(location = 0) in vec3 a_position;

out VS_OUT {
    vec3 texcoord;
} vs_out;

uniform mat4 u_view, u_projection;

void main() {
    vs_out.texcoord = a_position;

    const vec4 position = u_projection * u_view * vec4(a_position, 1.0f);
    gl_Position = position.xyww;
}