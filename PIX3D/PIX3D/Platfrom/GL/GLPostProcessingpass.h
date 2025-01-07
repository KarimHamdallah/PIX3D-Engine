#pragma once
#include "GLShader.h"

namespace PIX3D
{
	namespace GL
	{
		class GLPostProcessingpass
		{
		public:
			void Init(uint32_t width, uint32_t height);
			void Resize(uint32_t width, uint32_t height);
			void Render(uint32_t main_renderpass_color_attachment, uint32_t bloom_renderpass_output_color_attachment);

			uint32_t GetColorAttachment() { return m_ColorAttachment; }

		public:
			uint32_t m_Width, m_Height = 0;

			uint32_t m_Framebuffer = 0;
			uint32_t m_ColorAttachment = 0;
			uint32_t m_DepthStencilRenderbuffer = 0;
			GLShader m_PostProcessingShader;

			bool m_BloomEnabled = false;
			float m_BloomIntensity = 0.5f;
			bool m_TonemappingEnabled = false;
			float m_GammaCorrectionFactor = 2.2f;

			friend class GLRenderer;
			friend class LightningWidget;
		};
	}
}
