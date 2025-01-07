#pragma once
#include "GLVertexArray.h"
#include "GLFramebuffer.h"
#include "GLShader.h"

namespace PIX3D
{
	namespace GL
	{
		class GLScreenQuadRenderpass
		{
		public:
			static void Init();
			static void Destroy();
			static void Render(uint32_t texturehandle);
		private:
			// Screen Quad Pass
			inline static GLShader s_ScreenQuadShader;
		};
	}
}
