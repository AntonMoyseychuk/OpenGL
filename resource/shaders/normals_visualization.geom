#version 430 core

layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

in VS_OUT {
    vec3 normal;
} gs_in[];

layout (std140, binding = 0) uniform Matrices {
    uniform mat4 u_view;
    uniform mat4 u_projection;
};

void main() {
    const vec4 center = u_projection * ((gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position) / 3.0f);
    const vec4 normal = u_projection * vec4((gs_in[0].normal + gs_in[1].normal + gs_in[2].normal) / 3.0f, 0.0f);
    // const vec3 a = vec3(gl_in[0].gl_Position - gl_in[1].gl_Position);
    // const vec3 b = vec3(gl_in[2].gl_Position - gl_in[1].gl_Position);
    // const vec4 normal = u_projection * vec4(normalize(cross(a, b)), 0.0f);

    gl_Position = center;
    EmitVertex();
    gl_Position = center + vec4(normalize(vec3(normal)), 0.0f) * 0.5f;
    EmitVertex();
    EndPrimitive();
}