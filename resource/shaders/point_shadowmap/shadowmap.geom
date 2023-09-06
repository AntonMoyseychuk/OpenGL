#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 LightSpaceMatrices[6];

out vec4 FragPos;

void main() {
	for(int Face = 0; Face < 6; Face++)
	{
		gl_Layer = Face;
		for(int I = 0; I < 3; I++)
		{
			FragPos = gl_in[I].gl_Position;
			gl_Position = LightSpaceMatrices[Face] * FragPos;
			EmitVertex();		
		}
		EndPrimitive();
	}
}