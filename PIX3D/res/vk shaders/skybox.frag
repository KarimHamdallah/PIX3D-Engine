#version 460 core

#extension GL_KHR_vulkan_glsl : enable

#define GREYSCALE_WEIGHT_VECTOR vec3(0.2126, 0.7152, 0.0722)

layout (location = 0) out vec4 FragColor; // regular output
layout (location = 1) out vec4 BloomColor; // output to be used by bloom shader

layout (location = 0) in vec3 in_TextureCoords;

layout (set = 1, binding = 0) uniform samplerCube u_CubemapTexture;

layout(push_constant) uniform PushConstants {
    mat4 model;
   vec3 CameraPosition;
   float MeshIndex;
   float BloomThreshold;
}push;

void main()
{
	FragColor = texture(u_CubemapTexture, in_TextureCoords); // for samplerCube the coordinates are a vector

	// bloom color output
	// use greyscale conversion here because not all colors are equally "bright"
	float greyscaleBrightness = dot(FragColor.rgb, GREYSCALE_WEIGHT_VECTOR);
	BloomColor = greyscaleBrightness > push.BloomThreshold ? FragColor : vec4(0.0, 0.0, 0.0, 1.0);
}
