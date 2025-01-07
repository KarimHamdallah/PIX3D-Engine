#pragma once
#include <Graphics/GraphicsContext.h>

namespace PIX3D
{
	namespace GL
	{
		class GLGraphicsContext : public PIX3D::GraphicsContext
		{
		public:
			virtual void Init(void* NativeWindowHandle) override;
			virtual void SwapBuffers(void* NativeWindowHandle) override;
			virtual void EnableDepthTest(bool depthtest) override;
		};
	}
}
