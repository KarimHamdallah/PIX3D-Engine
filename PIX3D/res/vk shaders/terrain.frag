#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outBrightColor;

void main()
{
    // Simple orange color
    vec3 color = vec3(1.0, 0.65, 0.0); // Orange
    
    // Basic lighting calculation
    vec3 normal = normalize(inNormal);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(normal, lightDir), 0.2); // Minimum ambient of 0.2
    
    outColor = vec4(color * diff, 1.0);
    
    // No bloom for terrain
    outBrightColor = vec4(0.0);
}
