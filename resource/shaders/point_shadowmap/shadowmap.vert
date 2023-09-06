#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 Model = mat4(1.0);

void main()
{
	gl_Position = Model * vec4(aPos, 1.0);
}