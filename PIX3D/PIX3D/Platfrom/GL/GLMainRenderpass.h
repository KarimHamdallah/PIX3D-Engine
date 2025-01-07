#pragma once
#include <Core/Core.h>
#include "GLShader.h"

namespace PIX3D
{
	namespace GL
	{
		class GLMainRenderpass
		{
		public:
			void Init(uint32_t width, uint32_t height);
			void Resize(uint32_t width, uint32_t height);
			void Destroy();

			inline uint32_t GetFramebuffer() { return m_Framebuffer; }
			inline uint32_t GetColorAttachment() { return m_ColorAttachment; }
			inline uint32_t GetBloomColorAttachment() { return m_BloomColorAttachment; }

			void BindFrameBuffer(const glm::vec4& clear_color);
			void UnBindFrameBuffer();

		private:
			uint32_t m_Width, m_Height = 0;

			uint32_t m_Framebuffer = 0;
			uint32_t m_ColorAttachment = 0;
			uint32_t m_BloomColorAttachment = 0;
			uint32_t m_DepthStencilRenderbuffer = 0;
		};
	}
}
