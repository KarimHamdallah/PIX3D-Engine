#version 460 core

layout (location = 0) in vec2 in_Position;
layout (location = 1) in vec2 in_TextureCoords;

layout (location = 0) out vec2 out_TextureCoords;

void main()
{
	gl_Position = vec4(in_Position, 0.0, 1.0f); // input coordinates are already in NDC
	out_TextureCoords = in_TextureCoords;
}
