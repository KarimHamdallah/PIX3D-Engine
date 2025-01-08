#version 460

#extension GL_KHR_vulkan_glsl : enable

// out
layout(location = 0) out vec4 outColor;

// in
layout (location = 0) in vec2 in_TexCoords;

// resources
layout(binding = 0) uniform sampler2D TexSampler;


void main()
{
    outColor = texture(TexSampler, in_TexCoords);
}
