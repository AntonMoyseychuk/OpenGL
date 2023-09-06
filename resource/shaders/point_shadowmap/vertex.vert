#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out VS_OUT 
{
	vec3 FragmentPosWorld;
	vec3 Normal;
	vec2 TexCoord;
} vs_out;

uniform int ReverseNormals;

uniform mat4 Projection = mat4(1.0);
uniform mat4 View = mat4(1.0);
uniform mat4 Model = mat4(1.0);

void main()
{
	vs_out.FragmentPosWorld = vec3(Model * vec4(aPosition, 1.0));
	vs_out.Normal = normalize(mat3(Model) * (ReverseNormals * aNormal));
	vs_out.TexCoord = aTexCoord;

	gl_Position = Projection *  View *  Model * vec4(aPosition,  1.0);
}