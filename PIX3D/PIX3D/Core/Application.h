#pragma once
#include <string>

namespace PIX3D
{
	struct ApplicationSpecs
	{
		uint32_t Width = 800;
		uint32_t Height = 600;
		std::string Title = "PIX3D Application";
	};

	class Application
	{
	public:
		Application() = default;
		virtual ~Application() {}
		
		virtual void OnStart() {}
		virtual void OnUpdate(float dt) {}
		virtual void OnDestroy() {}
		virtual void OnResize(uint32_t width, uint32_t height) {}
		virtual void OnKeyPressed(uint32_t key) {}
	private:
	};
}
