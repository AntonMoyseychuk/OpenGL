#version 430 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform float u_time;

vec4 explode(vec4 position, vec4 normal, float magnitude) {
    const vec4 direction = normal * (sin((1.0f - u_time) / 4.0f) * 2.0f - 1.0f);
    return position + direction * magnitude;
}

void main() {
    const vec3 a = vec3(gl_in[0].gl_Position - gl_in[1].gl_Position);
    const vec3 b = vec3(gl_in[2].gl_Position - gl_in[1].gl_Position);
    const vec4 normal = vec4(normalize(cross(a, b)), 0.0f);

    gl_Position = explode(gl_in[0].gl_Position, normal, 2.0f);
    EmitVertex();
    gl_Position = explode(gl_in[1].gl_Position, normal, 2.0f);
    EmitVertex();
    gl_Position = explode(gl_in[2].gl_Position, normal, 2.0f);
    EmitVertex();
    EndPrimitive();
}