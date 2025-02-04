#version 450

#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outBrightColor;

layout(set = 1, binding = 0) uniform sampler2D _Texture;

void main()
{
    // Simple orange color
    vec3 color = texture(_Texture, inTexCoord * 100).rgb;
    
    // Basic lighting calculation
    vec3 normal = normalize(inNormal);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(normal, lightDir), 0.2); // Minimum ambient of 0.2
    
    outColor = vec4(color * diff, 1.0);
    
    // No bloom for terrain
    outBrightColor = vec4(0.0);
}
