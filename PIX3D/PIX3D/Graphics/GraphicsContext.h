#pragma once

namespace PIX3D
{
	class GraphicsContext
	{
	public:
		virtual void Init(void* NativeWindowHandle) {}
		virtual void SwapBuffers(void* NativeWindowHandle) {}
		virtual void EnableDepthTest(bool depthtest) {}
	};
}
