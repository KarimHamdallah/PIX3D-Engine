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
const bool TonemappingEnabled = false;
const float GammaCorrectionFactor = 2.2;

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

	// tonemapping
	if (TonemappingEnabled)
	{
		// apply Reinhard tonemapping C = C / (1 + C)
		color = color / (color + vec3(1.0));
	}

	// gamma correction
	color = pow(color, vec3(1.0 / GammaCorrectionFactor)); // gamma correction to account for monitor, raise to the (1 / 2.2)

	outColor = vec4(color, 1.0);
}
