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

				s_DefaultAlbedoTexture.Create();
				s_DefaultAlbedoTexture.LoadFromData(data.data(), 2, 2, TextureFormat::RGBA8);
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

				s_DefaultNormalTexture.Create();
				s_DefaultNormalTexture.LoadFromData(data.data(), 2, 2, TextureFormat::RGBA8);
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

				s_DefaultWhiteTexture.Create();
				s_DefaultWhiteTexture.LoadFromData(data.data(), 2, 2, TextureFormat::RGBA8);
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

				s_DefaultBlackTexture.Create();
				s_DefaultBlackTexture.LoadFromData(data.data(), 2, 2, TextureFormat::RGBA8);
			}
		}
	}
}
