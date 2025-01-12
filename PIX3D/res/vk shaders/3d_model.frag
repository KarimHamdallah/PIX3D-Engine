#version 460

#extension GL_KHR_vulkan_glsl : enable

#define GREYSCALE_WEIGHT_VECTOR vec3(0.2126, 0.7152, 0.0722)

// out
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightnessColor;

// in
layout (location = 0) in vec2 in_TexCoords;

// resources
layout(set = 1, binding = 0) uniform sampler2D AlbedoTexture;
layout(set = 1, binding = 1) uniform sampler2D EmissiveTexture;


// Environment Mapping Set
layout(set = 2, binding = 0) uniform samplerCube DiffuseIrradianceMap;
layout(set = 2, binding = 1) uniform samplerCube PrefilteredEnvMap;
layout(set = 2, binding = 2) uniform samplerCube BrdfConvolutionMap;

const float BloomBrightnessCutoff = 1.0;

void main()
{

    vec3 albedo = texture(AlbedoTexture, in_TexCoords).rgb;
    vec3 emissive = texture(EmissiveTexture, in_TexCoords).rgb;

    vec3 color = albedo + emissive;

    FragColor = vec4(color, 1.0);

    // bloom color output
	// use greyscale conversion here because not all colors are equally "bright"
	vec3 brightColor = emissive;
    float greyscaleBrightness = dot(FragColor.rgb, GREYSCALE_WEIGHT_VECTOR);
	BrightnessColor = greyscaleBrightness > BloomBrightnessCutoff ? vec4(brightColor, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
}
