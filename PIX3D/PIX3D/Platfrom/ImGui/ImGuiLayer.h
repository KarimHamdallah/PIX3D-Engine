#pragma once

namespace PIX3D
{
	class ImGuiLayer
	{
	public:
		static void Init();
		static void Destroy();
		static void BeginDraw();
		static void EndDraw();

		static void StartDockSpace();
		static void EndDockSpace();
	private:

	};
}
