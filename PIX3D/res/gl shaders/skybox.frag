#version 460 core

#define GREYSCALE_WEIGHT_VECTOR vec3(0.2126, 0.7152, 0.0722)

layout (location = 0) out vec4 FragColor; // regular output
layout (location = 1) out vec4 BloomColor; // output to be used by bloom shader

layout (location = 0) in vec3 in_TextureCoords;

layout (location = 1) uniform samplerCube u_CubemapTexture;
layout (location = 2) uniform float u_BloomBrightnessCutoff;

void main() {
	FragColor = texture(u_CubemapTexture, in_TextureCoords); // for samplerCube the coordinates are a vector

	// bloom color output
	// use greyscale conversion here because not all colors are equally "bright"
	float greyscaleBrightness = dot(FragColor.rgb, GREYSCALE_WEIGHT_VECTOR);
	BloomColor = greyscaleBrightness > u_BloomBrightnessCutoff ? FragColor : vec4(0.0, 0.0, 0.0, 1.0);
}
