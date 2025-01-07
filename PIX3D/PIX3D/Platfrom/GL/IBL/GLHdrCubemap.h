#pragma once
#include <filesystem>
#include "../GLObject.h"

namespace PIX3D
{
	namespace GL
	{
		class GLHdriCubemap : public GLObject
		{
		public:
            GLHdriCubemap() = default;
			void Load(uint32_t cubemaphandle);
            void LoadHdrToCubemapCPU(const std::filesystem::path& path, uint32_t size);
			void LoadHdrToCubemapGPU(const std::filesystem::path& path, uint32_t size);
			void Test();
			void Destroy();
			void Bind() const;

			uint32_t GetHdrTextureHandle() { return m_EquiRectangularHdrTextureHandle; }

		private:
			uint32_t m_EquiRectangularHdrTextureHandle = 0;
		};
	}
}
