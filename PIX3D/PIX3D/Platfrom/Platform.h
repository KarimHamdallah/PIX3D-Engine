#pragma once
#include <vector>
#include <stdint.h>
#include <string>
#include <filesystem>

namespace PIX3D
{
	enum class FileDialougeFilter
	{
		PIXPROJ,
		PIXSCENE,
		PATH,
		PNG, HDR,
		GLTF
	};


	class PlatformLayer
	{
	public:
		PlatformLayer() = default;
		~PlatformLayer() {}

		virtual void CreatWindow(uint32_t width, uint32_t height, const char* name, bool resizable) {}
		virtual void PollEvents() {}
		virtual void* GetNativeWindowHandel() { return nullptr; }

		virtual std::pair<uint32_t, uint32_t> GetWindowSize() { return { 0.0f, 0.0f }; }

		virtual bool IsKeyPressed(int keycode) { return false; }
		virtual bool IsKeyReleased(int keycode) { return false; }

		virtual bool IsMouseButtonPressed(int mousebutton) { return false; }
		virtual bool IsMouseButtonReleased(int mousebutton) { return false; }

		// Plaform Utils
		// Image Utils
		virtual void ExportImagePNG(const std::string& path, uint32_t width, uint32_t height, const std::vector<uint8_t>& pixels, uint32_t bpp = 4) { }

		// Dialogue
		virtual std::filesystem::path OpenDialogue(FileDialougeFilter Filter) { return std::filesystem::path(); }
		virtual std::filesystem::path SaveDialogue(FileDialougeFilter Filter) { return std::filesystem::path(); }

		virtual void ShowCursor(bool show) {}
	};
}
