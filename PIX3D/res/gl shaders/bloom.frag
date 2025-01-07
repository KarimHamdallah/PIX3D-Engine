#version 460 core

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 in_TextureCoords;

// first weight is for the central pixel, the remaining are on either side of it
// meaning we are working with a 9x9 kernel
const float gaussianBlurWeights[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

layout (location = 0) uniform sampler2D u_InputColorTexture;
layout (location = 1) uniform int u_SampleMipLevel;
layout (location = 2) uniform vec2 u_BlurDirection;

void main()
{
	// size of 1 pixel in [0-1] coordinates
	vec2 texelSize = 1.0 / textureSize(u_InputColorTexture, u_SampleMipLevel);

	vec3 result = vec3(0.0, 0.0, 0.0);

	// center pixel
	result += textureLod(u_InputColorTexture, in_TextureCoords, u_SampleMipLevel).rgb * gaussianBlurWeights[0];

	for (int i = 1; i < 5; i++) {
		vec2 sampleOffset = vec2(
			texelSize.x * i * u_BlurDirection.x,
			texelSize.y * i * u_BlurDirection.y
		);

		result += texture(u_InputColorTexture, in_TextureCoords + sampleOffset).rgb * gaussianBlurWeights[i];
		result += texture(u_InputColorTexture, in_TextureCoords - sampleOffset).rgb * gaussianBlurWeights[i];
	}

	FragColor = vec4(result, 1.0);
}
