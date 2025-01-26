#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 in_TexCoords;

layout(set = 0, binding = 0) uniform sampler2D SceneColorTexture;
layout(set = 0, binding = 1) uniform sampler2D GlassMaskTexture;

void main()
{
    vec4 sceneColor = texture(SceneColorTexture, in_TexCoords);
    vec4 glassMask = texture(GlassMaskTexture, in_TexCoords);
    

    // Check if this pixel is part of the glass mask (cyan color)
    if(glassMask.rgb == vec3(0.0, 1.0, 1.0))
    {
    	sceneColor.rgb = pow(sceneColor.rgb, vec3(1.0 / 2.2));
        FragColor = sceneColor;
    }
    else
    {
        FragColor = vec4(0.0);  // Transparent black
    }
}
