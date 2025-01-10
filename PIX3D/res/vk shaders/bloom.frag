#version 460 core
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 in_TextureCoords;

layout(push_constant) uniform PushConstants {
    vec2 direction;
    int mipLevel;
    int useInputTexture;
} push;

layout(set = 0, binding = 0) uniform sampler2D otherBufferTex;

// Gaussian blur weights for 9x9 kernel
const float gaussianBlurWeights[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    // size of 1 pixel in [0-1] coordinates
     vec2 texelSize = vec2(0.0, 0.0);
     texelSize = 1.0 / textureSize(otherBufferTex, push.mipLevel);
     vec3 result = vec3(0.0, 0.0, 0.0);
     
     // center pixel
     result += textureLod(otherBufferTex, in_TextureCoords, push.mipLevel).rgb * gaussianBlurWeights[0];
     
     // sample neighboring pixels
     for (int i = 1; i < 5; i++) {
         vec2 sampleOffset = vec2(
             texelSize.x * i * push.direction.x,
             texelSize.y * i * push.direction.y
         );
        
    
     result += texture(otherBufferTex, in_TextureCoords + sampleOffset).rgb * gaussianBlurWeights[i];
     result += texture(otherBufferTex, in_TextureCoords - sampleOffset).rgb * gaussianBlurWeights[i];
    }
    
    FragColor = vec4(result, 1.0);
}
