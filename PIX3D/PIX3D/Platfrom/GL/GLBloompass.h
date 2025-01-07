#pragma once
#include "GLShader.h"

namespace PIX3D
{
	namespace GL
	{
		enum GLBloomDirection
		{
			BOTH = 0,
			HORIZONTAL,
			VERTICAL
		};

		class GLBloompass
		{
		public:
			void Init(uint32_t width, uint32_t height);
			void Resize(uint32_t width, uint32_t height);
			void Render(uint32_t bloom_brightness_color_attachment);
			void SetMipLevels(int miplevel); // Choose Which MipLevel As The Render Target
			void Destroy();

			uint32_t GetOutputTexture() { return m_ColorAttachments[m_BloomFramebufferResult]; }
			int GetBloomDirection() { return m_BloomDirection; }
			void SetBloomDirection(GLBloomDirection direction) { m_BloomDirection = direction; }
			uint32_t GetBloomIterations() { return m_BloomIterations; }
			void SetBloomIterations(uint32_t iterations) { m_BloomIterations = iterations; }

		public:
			uint32_t m_Width, m_Height = 0;

			GLShader m_BloomShader;
			std::vector<uint32_t> m_Framebuffers;
			std::vector<uint32_t> m_ColorAttachments;
			int m_BloomDirection = GLBloomDirection::BOTH;
			int m_BloomIterations = 10;
			uint32_t m_BloomFramebufferResult = 0;
		};
	}
}
