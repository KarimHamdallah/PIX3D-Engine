#include "VulkanSceneRenderer.h"


namespace PIX3D
{
	namespace VK
	{
		void VulkanSceneRenderer::Init()
		{
			// Default Albedo Texture
			{
				// RGBA8 Texture
				std::vector<uint8_t> data =
				{
					255, 0, 255, 255,
					255, 0, 255, 255,
					255, 0, 255, 255,
					255, 0, 255, 255
				};

				s_DefaultAlbedoTexture = new VulkanTexture();

				s_DefaultAlbedoTexture->Create();
				s_DefaultAlbedoTexture->LoadFromData(data.data(), 2, 2, TextureFormat::RGBA8);
			}

			// Default Normal Texture
			{
				// RGBA8 Texture
				std::vector<uint8_t> data =
				{
					127, 127, 255, 255,
					127, 127, 255, 255,
					127, 127, 255, 255,
					127, 127, 255, 255
				};

				s_DefaultNormalTexture = new VulkanTexture();

				s_DefaultNormalTexture->Create();
				s_DefaultNormalTexture->LoadFromData(data.data(), 2, 2, TextureFormat::RGBA8);
			}

			// Default White Texture
			{
				// RGBA8 Texture
				std::vector<uint8_t> data =
				{
					255, 255, 255, 255,
					255, 255, 255, 255,
					255, 255, 255, 255,
					255, 255, 255, 255
				};

				s_DefaultWhiteTexture = new VulkanTexture();

				s_DefaultWhiteTexture->Create();
				s_DefaultWhiteTexture->LoadFromData(data.data(), 2, 2, TextureFormat::RGBA8);
			}

			// Default Black Texture
			{
				// RGBA8 Texture
				std::vector<uint8_t> data =
				{
					0, 0, 0, 255,
					0, 0, 0, 255,
					0, 0, 0, 255,
					0, 0, 0, 255
				};

				s_DefaultBlackTexture = new VulkanTexture();

				s_DefaultBlackTexture->Create();
				s_DefaultBlackTexture->LoadFromData(data.data(), 2, 2, TextureFormat::RGBA8);
			}

			////////////////// Vulkan Static Mesh Material Descriptor Set Layout ///////////////////////
			s_VulkanStaticMeshMaterialDescriptorSetLayout
				.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.Build();
		}

		void VulkanSceneRenderer::Destroy()
		{
			s_DefaultAlbedoTexture->Destroy();
			s_DefaultNormalTexture->Destroy();
			s_DefaultWhiteTexture->Destroy();
			s_DefaultBlackTexture->Destroy();

			delete s_DefaultAlbedoTexture;
			delete s_DefaultNormalTexture;
			delete s_DefaultWhiteTexture;
			delete s_DefaultBlackTexture;
		}
	}
}
