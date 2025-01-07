#version 460 core

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 in_texcoords;

layout (location = 0) uniform sampler2D framebuffer_texture;

void main()
{
    FragColor = texture(framebuffer_texture, in_texcoords);
}
