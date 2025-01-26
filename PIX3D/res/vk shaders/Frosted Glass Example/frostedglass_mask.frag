#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) out vec4 FragColor;

void main()
{
    FragColor = vec4(0.0, 1.0, 1.0, 1.0); // Cyan color
}
