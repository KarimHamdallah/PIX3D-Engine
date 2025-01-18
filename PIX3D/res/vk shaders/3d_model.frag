#version 460

#extension GL_KHR_vulkan_glsl : enable

#define PI 3.1415926535897932384626433832795
#define GREYSCALE_WEIGHT_VECTOR vec3(0.2126, 0.7152, 0.0722)

// out
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BloomColor; // output to be used by bloom shader

// in
layout (location = 0) in vec2 in_TexCoords;
layout (location = 1) in vec3 in_WorldCoordinates;
layout (location = 2) in vec3 in_Tangent;
layout (location = 3) in vec3 in_BiTangent;
layout (location = 4) in vec3 in_Normal;

struct MaterialGPUShader_InfoBufferData
{
	bool UseAlbedoTexture;
	bool UseNormalTexture;
	bool UseMetallicRoughnessTexture;
	bool useAoTexture;
	bool UseEmissiveTexture;

	vec3 BaseColor;
	vec3 EmissiveColor;

	float Metalic;
	float Roughness;
	float Ao;
	bool UseIBL;
};


// Material Set
layout(std430, set = 1, binding = 0) readonly buffer MaterialsInfoBuffer
{
	 MaterialGPUShader_InfoBufferData Data[];
}material_info;

layout(set = 1, binding = 1) uniform sampler2D AlbedoTexture;
layout(set = 1, binding = 2) uniform sampler2D NormalTexture;
layout(set = 1, binding = 3) uniform sampler2D MetalRoughnessTexture;
layout(set = 1, binding = 4) uniform sampler2D AoTexture;
layout(set = 1, binding = 5) uniform sampler2D EmissiveTexture;

// Environment Mapping Set
layout(set = 2, binding = 0) uniform samplerCube DiffuseIrradianceMap;
layout(set = 2, binding = 1) uniform samplerCube PrefilteredEnvMap;
layout(set = 2, binding = 2) uniform sampler2D BrdfConvolutionMap;


layout(push_constant) uniform PushConstants
{
   mat4 model;
   vec3 CameraPosition;
   float MeshIndex;
   float BloomThreshold;
   float PointLightCount;
}push;


////////////////// Point Lights /////////////////////////


struct PointLightStruct
{
    vec4 LightPosition;
	vec4 LightColor;
	float Intensity;  // Controls the brightness of the light
    float Radius;     // Controls the maximum range of the light
	float Falloff;      // Controls the falloff curve
};


layout(std430, set = 3, binding = 0) readonly buffer PointLightsBuffer
{
	 PointLightStruct Data[];
}pointlights_info;


// PBR
// IBL precomputed maps
const float PREFILTERED_ENV_MAP_LOD = 5.0; // how many mipmap levels

// Fresnel function (Fresnel-Schlick approximation)
//
// F_schlick = f0 + (1 - f0)(1 - (h * v))^5
//
vec3 fresnelSchlick(float cosTheta, vec3 f0)
{ 
	return f0 + (1.0 - f0) * pow(max(1 - cosTheta, 0.0), 5.0);
}

// Fresnel schlick roughness
//
// Same as above except with a roughness term
vec3 fresnelSchlickRoughness(float cosTheta, vec3 f0, float roughness)
{
	return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Normal distribution function (Trowbridge-Reitz GGX)
//
//                alpha ^ 2
//     ---------------------------------
//      PI((n * h)^2(alpha^2 - 1) + 1)^2
//
float ndfTrowbridgeReitzGGX(vec3 n, vec3 h, float roughness)
{	
	float alpha = roughness * roughness; // recommended by disney/epic papers
	float alphaSquared = alpha * alpha;

	float nDotH = max(dot(n, h), 0.0);
	float nDotHSquared = nDotH * nDotH;
	float innerTerms = nDotHSquared * (alphaSquared - 1.0) + 1.0;

	float numerator = alphaSquared;
	float denomenator = PI * innerTerms * innerTerms;
	denomenator = max(denomenator, 0.0001); // avoid div by zero

	return numerator / denomenator;
}

// Geometry function
//
//         n * v
//   -------------------
//   (n * v)(1 - k) + k
//
float geometrySchlickGGX(vec3 n, vec3 v, float k)
{

	float nDotV = max(dot(n, v), 0.0);

	float numerator = nDotV;
	float denomenator = nDotV * (1.0 - k) + k;

	return numerator / denomenator;
}

// smiths method for taking into account view direction and light direction
float geometrySmith(vec3 n, vec3 v, vec3 l, float roughness)
{

    // remapping for direct lighting (doesn't work for IBL)
	float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;

	return geometrySchlickGGX(n, v, k) * geometrySchlickGGX(n, l, k);
}

// Tangent space to world
vec3 calculateNormal(vec3 tangentNormal)
{
	vec3 norm = normalize(tangentNormal * 2.0 - 1.0);
	mat3 TBN  = mat3(in_Tangent, in_BiTangent, in_Normal);
	return normalize(TBN * norm); // tangent --> world
}

/*
vec3 CalculateDirectionalLight(vec3 n, vec3 v, vec3 albedo, float metallic, float roughness, vec3 f0)
{
    vec3 Lo = vec3(0.0);
    
    //for(int i = 0; i < u_DirLightsCount; i++)
    {
        vec3 l = normalize(-u_DirLightDirection.xyz);
        vec3 h = normalize(v + l);
        
        // Calculate radiance (no attenuation for directional lights)
        vec3 radiance = u_DirLightColor.rgb * u_DirLightIntensity;
        
        // Cook-Torrance BRDF
        float NDF = ndfTrowbridgeReitzGGX(n, h, roughness);
        vec3 F = fresnelSchlick(max(dot(h, v), 0.0), f0);
        float G = geometrySmith(n, v, l, roughness);
        
        vec3 numerator = NDF * F * G;
        float denominator = max(4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0), 0.001);
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        float NdotL = max(dot(n, l), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    return Lo;
}
*/

void main()
{
	// retrieve all the material properties
	//MaterialGPUShader_TextureBufferData MaterialTextureData = material_textures.Data[int(u_MeshIndex)];
	MaterialGPUShader_InfoBufferData MaterialInfoData = material_info.Data[int(push.MeshIndex)];

	// albedo
	vec3 albedo = MaterialInfoData.BaseColor.rgb;
	if (MaterialInfoData.UseAlbedoTexture)
	{
		albedo = texture(AlbedoTexture, in_TexCoords).rgb;
	}

	// metallic/roughness
	float metallic = MaterialInfoData.Metalic;
	float roughness = MaterialInfoData.Roughness;
	if (MaterialInfoData.UseMetallicRoughnessTexture)
	{
		vec3 metallicRoughness = texture(MetalRoughnessTexture, in_TexCoords).rgb;
		metallic = metallicRoughness.b;
		roughness = metallicRoughness.g;
	}

	// normal
	vec3 n = in_Normal; // interpolated vertex normal
	if (MaterialInfoData.UseNormalTexture)
	{
		n = calculateNormal(texture(NormalTexture, in_TexCoords).rgb);
	}

	// ambient occlusion
	float ao = MaterialInfoData.Ao;
	if (MaterialInfoData.useAoTexture)
	{
		ao = texture(AoTexture, in_TexCoords).r;
	}

	// emissive
	vec3 emissive = MaterialInfoData.EmissiveColor.rgb;
	if (MaterialInfoData.UseEmissiveTexture)
	{
		emissive = texture(EmissiveTexture, in_TexCoords).rgb;
	}

	vec3 v = normalize(push.CameraPosition - in_WorldCoordinates); // view vector pointing at camera
	vec3 r = reflect(-v, n); // reflection

	// f0 is the "surface reflection at zero incidence"
	// for PBR-metallic we assume dialectrics all have 0.04
	// for metals the value comes from the albedo map
	vec3 f0 = vec3(0.04);
	f0 = mix(f0, albedo, metallic);

	vec3 Lo = vec3(0.0); // total radiance out

	// Direct lighting
	
	// Sum up the radiance contributions of each light source.
	// This loop is essentially the integral of the rendering equation.

	int PointLightsCount = int(push.PointLightCount);
	for (int i = 0; i < PointLightsCount; i++)
	{
		vec3 l = normalize(pointlights_info.Data[i].LightPosition.xyz - in_WorldCoordinates); // light vector
		vec3 h = normalize(v + l);

		float distance = length(pointlights_info.Data[i].LightPosition.xyz - in_WorldCoordinates);
		//float attenuation = 1.0 / (distance * distance); // inverse square law

		/////////////

		// Calculate attenuation using radius
        float attenuation = clamp(1.0 - (distance * distance) / (pointlights_info.Data[i].Radius * pointlights_info.Data[i].Radius), 0.0, 1.0);
        attenuation *= mix(attenuation, 1.0, pointlights_info.Data[i].Falloff);

		////////////



		vec3 radiance = pointlights_info.Data[i].LightColor.xyz * pointlights_info.Data[i].Intensity * attenuation; // aka Li

		// calculate Cook-Torrance specular BRDF term
		//
		//                DFG
		//        --------------------
		//         4(w_0 * n)(w_i * n)
		//
		//

		// Normal Distribution term (D)
		float dTerm = ndfTrowbridgeReitzGGX(n, h, roughness);

		// Fresnel term (F)
		// Determines the ratio of light reflected vs. absorbed
		vec3 fTerm = fresnelSchlick(max(dot(h, v), 0.0), f0);

		// Geometry term (G)
		float gTerm = geometrySmith(n, v, l, roughness);


		vec3 numerator = dTerm * fTerm * gTerm;
		float denominator = 4.0 * max(dot(v, n), 0.0) * max(dot(l, n), 0.0);
		
		// recall fTerm is the proportion of reflected light, so the result here is the specular
		vec3 specular = numerator / max(denominator, 0.001);

		vec3 kSpecular = fTerm;
		vec3 kDiffuse = vec3(1.0) - kSpecular;
		kDiffuse *= 1.0 - metallic; // metallic materials should have no diffuse component

		// now calculate full Cook-Torrance with both diffuse + specular
		//
		// f_r = kd * f_lambert + ks * f_cook-torrance
		//
		// where f_lambert = c / pi

		vec3 diffuse = kDiffuse * albedo / PI;
		vec3 cookTorranceBrdf = diffuse + specular;
		float nDotL = max(dot(n, l), 0.0);

		// Finally, the rendering equation!
		Lo += cookTorranceBrdf * radiance * nDotL;
	}

	// Add directional lights contribution
    /*
	if(u_HasDirLight)
	{
	    Lo += CalculateDirectionalLight(n, v, albedo, metallic, roughness, f0);
	}
	*/

	vec3 color = vec3(0.0);

	if(MaterialInfoData.UseIBL)
	{
	    // Indirect lighting (IBL)
	    vec3 kSpecular = fresnelSchlickRoughness(max(dot(n, v), 0.0), f0, roughness); // aka F
        vec3 kDiffuse = 1.0 - kSpecular;
	    kDiffuse *= 1.0 - metallic; // metallic materials should have no diffuse component
	    
	    // diffuse
        vec3 irradiance = texture(DiffuseIrradianceMap, n).rgb;
        vec3 diffuse = irradiance * albedo;
	    
	    // specular
	    vec3 prefilteredEnvMapColor = textureLod(PrefilteredEnvMap, r, roughness * PREFILTERED_ENV_MAP_LOD).rgb;
	    float NdotV = max(dot(n, v), 0.0);
	    vec2 brdf = texture(BrdfConvolutionMap, vec2(NdotV, roughness)).rg;
	    vec3 specular = prefilteredEnvMapColor * (kSpecular * brdf.x + brdf.y);
	    
	    vec3 ambient = (kDiffuse * diffuse + specular) * ao; // indirect lighting

	    // Combine emissive + indirect + direct
	    color = emissive + ambient + Lo;
	}
	else
	{
	    // Combine emissive + direct
	    color = emissive + Lo;
	}

	// Outputs

	// main color output
	FragColor = vec4(color, 1.0);

	// bloom color output
	// use greyscale conversion here because not all colors are equally "bright"
	vec3 brightColor = emissive + Lo * 0.5;  // Include direct lighting contribution
    float greyscaleBrightness = dot(FragColor.rgb, GREYSCALE_WEIGHT_VECTOR);
	//BloomColor = greyscaleBrightness > u_BloomBrightnessCutoff ? vec4(emissive, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
	BloomColor = greyscaleBrightness > push.BloomThreshold ? vec4(brightColor, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
}
