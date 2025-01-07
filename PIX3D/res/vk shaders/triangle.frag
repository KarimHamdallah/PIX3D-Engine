#version 460

#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec4 outColor;

// in
layout (location = 0) in vec3 in_Color;


void main()
{
    outColor = vec4(in_Color, 1.0);
}
