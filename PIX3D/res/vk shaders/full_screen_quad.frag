#version 460

#extension GL_KHR_vulkan_glsl : enable

// out
layout(location = 0) out vec4 outColor;

// in
layout (location = 0) in vec2 in_TexCoords;

// resources
layout(set = 0, binding = 0) uniform sampler2D ColorAttachment;
layout(set = 0, binding = 1) uniform sampler2D BloomAttachment;

const bool BloomEnabled = true;
const float BloomIntensity = 1.0;

void main()
{
    vec3 color = texture(ColorAttachment, in_TexCoords).rgb;

	// bloom
	if (BloomEnabled)
	{
		vec3 bloomColor = vec3(0.0, 0.0, 0.0);
		for (int i = 0; i <= 5; i++)
		{
			bloomColor += textureLod(BloomAttachment, in_TexCoords, i).rgb;
		}

		color += bloomColor * BloomIntensity;
	}

	outColor = vec4(color, 1.0);
}
