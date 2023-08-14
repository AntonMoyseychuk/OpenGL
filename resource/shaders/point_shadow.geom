#version 430 core
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

out GEOM_OUT {
    vec4 frag_pos;
} gs_out;

uniform mat4 u_layer_matrices[6];

void main() {
    for (int face = 0; face < 6; ++face) {
        gl_Layer = face;
        for (int i = 0; i < 3; ++i) {
            gs_out.frag_pos = gl_in[i].gl_Position;
            gl_Position = u_layer_matrices[face] * gs_out.frag_pos;
            EmitVertex();
        }
        EndPrimitive();
    }
}