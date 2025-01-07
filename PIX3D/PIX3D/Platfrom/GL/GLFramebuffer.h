#pragma once
#include "GLObject.h"
#include <vector>

namespace PIX3D
{
	namespace GL
	{
		struct GLFramebufferSpecification
		{
			uint32_t Width = 800;
			uint32_t Height = 600;
			uint32_t SamplesCount = 8;
		};

		class GLFramebuffer : GLObject
		{
		public:
			GLFramebuffer() = default;
			~GLFramebuffer();

			void Create(const GLFramebufferSpecification& specs);
			void Begin();
			void End();

			void Resize(uint32_t width, uint32_t height);

			std::vector<uint8_t> GetPixels(uint32_t bpp = 4);
			std::vector<uint32_t> GetPixelsUint32();
			void SetPixelsUint32(const std::vector<uint32_t>& pixels);

			GLFramebufferSpecification GetFramebufferSpecs() { return m_Specs; }

			uint32_t GetColorAttachmentHandle() { return m_ColorAttachmentHandle; }

			operator const uint32_t() const
			{
				return m_ColorAttachmentHandle;
			}

		private:
			GLFramebufferSpecification m_Specs;
			uint32_t m_RenderbufferHandle = 0;
			uint32_t m_ColorAttachmentHandle = 0;
		};
	}
}
