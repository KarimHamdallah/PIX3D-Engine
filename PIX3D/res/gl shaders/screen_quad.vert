#version 460 core

layout (location = 0) in vec2 in_position;
layout (location = 1) in vec2 in_texcoords;


layout (location = 0) out vec2 out_texcoords;

void main()
{
    out_texcoords = in_texcoords;
    gl_Position = vec4(in_position, 0.0, 1.0);
}
