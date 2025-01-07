#pragma once
#include "GLObject.h"
#include <filesystem>

namespace PIX3D
{
	namespace GL
	{
		class GLTexture : public GLObject
		{
		public:
			GLTexture() = default;
			~GLTexture();

			void LoadFromData(uint8_t* data, uint32_t width, uint32_t height);
			void LoadFromFile(const std::string& path, bool srgb = false);
			void Bind(uint32_t slot = 0);
			
			void Destroy();

			inline uint64_t GetVramTextureID() const { return m_TextureID; }
			inline std::filesystem::path GetPath() const { return m_Path; }

		private:
			uint64_t m_TextureID = 0; // Vram Texture ID
			std::filesystem::path m_Path;
		};
	}
}
