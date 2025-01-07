#pragma once
#include "Platform.h"

namespace PIX3D
{
	class WindowsPlatformLayer : public PlatformLayer
	{
	public:
		WindowsPlatformLayer() = default;
		virtual ~WindowsPlatformLayer();

		virtual void CreatWindow(uint32_t width, uint32_t height, const char* title, bool resizable) override;
		virtual void PollEvents() override;
		virtual void* GetNativeWindowHandel() override;

		virtual std::pair<uint32_t, uint32_t> GetWindowSize() override;

		virtual bool IsKeyPressed(int keycode) override;
		virtual bool IsKeyReleased(int keycode) override;
		
		virtual bool IsMouseButtonPressed(int mousebutton) override;
		virtual bool IsMouseButtonReleased(int mousebutton) override;

		virtual void ExportImagePNG(const std::string& path, uint32_t width, uint32_t height, const std::vector<uint8_t>& pixels, uint32_t bpp = 4) override;

		virtual std::filesystem::path OpenDialogue(FileDialougeFilter Filter) override;
		virtual std::filesystem::path SaveDialogue(FileDialougeFilter Filter) override;

		virtual void ShowCursor(bool show) override; // called every frame

	private:
		void* m_NativeWindowHandel = nullptr;
	};
}
