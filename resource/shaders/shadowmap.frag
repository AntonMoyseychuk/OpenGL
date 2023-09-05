#version 330
in vec4 FragPos;

uniform vec3 LightPos;
uniform float FarPlane;

void main() {
	float Distance = length(LightPos - FragPos.xyz);
	Distance = Distance / FarPlane;

	gl_FragDepth = Distance;
}