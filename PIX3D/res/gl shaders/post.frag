#version 460 core

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 in_TexCoords;

layout (location = 0) uniform sampler2D u_ColorTexture;
layout (location = 1) uniform sampler2D u_BloomTexture;
layout (location = 2) uniform bool u_BloomEnabled;
layout (location = 3) uniform float u_BloomIntensity;
layout (location = 4) uniform bool u_TonemappingEnabled;
layout (location = 5) uniform float u_GammaCorrectionFactor;

void main()
{
	vec3 color = texture(u_ColorTexture, in_TexCoords).rgb;

	// bloom
	if (u_BloomEnabled)
	{
		vec3 bloomColor = vec3(0.0, 0.0, 0.0);
		for (int i = 0; i <= 5; i++)
		{
			bloomColor += textureLod(u_BloomTexture, in_TexCoords, i).rgb;
		}

		color += bloomColor * u_BloomIntensity;
	}

	// tonemapping
	if (u_TonemappingEnabled)
	{
		// apply Reinhard tonemapping C = C / (1 + C)
		color = color / (color + vec3(1.0));
	}

	// gamma correction
	color = pow(color, vec3(1.0 / u_GammaCorrectionFactor)); // gamma correction to account for monitor, raise to the (1 / 2.2)

	FragColor = vec4(color, 1.0);
}
