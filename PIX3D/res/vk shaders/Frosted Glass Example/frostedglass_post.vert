#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec2 in_Position;
layout (location = 1) in vec2 in_TextureCoords;

layout (location = 0) out vec2 out_TexCoords;

void main()
{
    gl_Position = vec4(in_Position, 0.0, 1.0);
    out_TexCoords = in_TextureCoords;
}
