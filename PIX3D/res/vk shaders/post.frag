#version 460

#extension GL_KHR_vulkan_glsl : enable

// out
layout(location = 0) out vec4 outColor;

// in
layout (location = 0) in vec2 in_TexCoords;

// resources
layout(set = 0, binding = 0) uniform sampler2D ColorAttachment;
layout(set = 0, binding = 1) uniform sampler2D BloomAttachment;
layout(set = 0, binding = 2) uniform sampler2D FrostedGlassBlurAttachment;

layout(push_constant) uniform PushConstants
{
   bool BloomEnabled;// = true;
   float BloomIntensity;// = 1.0;
   bool TonemappingEnabled;// = false;
   float GammaCorrectionFactor;// = 2.2;
   bool ForstedPassEnabled;// = false;
}push;

void main()
{
    vec3 color = texture(ColorAttachment, in_TexCoords).rgb;

	// bloom
	if (push.BloomEnabled)
	{
		vec3 bloomColor = vec3(0.0, 0.0, 0.0);
		for (int i = 0; i <= 5; i++)
		{
			bloomColor += textureLod(BloomAttachment, in_TexCoords, i).rgb;
		}

		color += bloomColor * push.BloomIntensity;
	}

	// tonemapping
	if (push.TonemappingEnabled)
	{
		// apply Reinhard tonemapping C = C / (1 + C)
		color = color / (color + vec3(1.0));
	}

	// gamma correction
	color = pow(color, vec3(1.0 / push.GammaCorrectionFactor)); // gamma correction to account for monitor, raise to the (1 / 2.2)

	outColor = vec4(color, 1.0);

	// Frosted Glass
	if(push.ForstedPassEnabled)
	{
	   vec3 FrostedGlassBlurColor = texture(FrostedGlassBlurAttachment, in_TexCoords).rgb;
	   
	   if(FrostedGlassBlurColor != vec3(0.0))
	   {
	       outColor.rgb = FrostedGlassBlurColor;
	   }
	}
}
