#version 460 core

layout (location = 0) in vec3 in_Position;

layout (location = 0) out vec3 out_ModelCoordinates;

layout (location = 0) uniform mat4 model;
layout (location = 1) uniform mat4 view;
layout (location = 2) uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(in_Position, 1.0f);
	out_ModelCoordinates = in_Position;
}
