#version 460

#extension GL_KHR_vulkan_glsl : enable

// out
layout(location = 0) out vec4 outColor1;
layout(location = 1) out vec4 outColor2;

// in
layout (location = 0) in vec2 in_TexCoords;

// resources
layout(set = 1, binding = 0) uniform sampler2D TexSampler;

void main()
{
    outColor1 = texture(TexSampler, in_TexCoords);
    outColor2 = vec4(0.0, 0.0, 1.0, 1.0);
}
