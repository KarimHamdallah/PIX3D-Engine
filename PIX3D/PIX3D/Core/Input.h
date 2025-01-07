#pragma once
#include "Key.h"
#include <glm/glm.hpp>

namespace PIX3D
{
	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode keycode);
		static bool IsKeyReleased(KeyCode keycode);

		static bool IsMouseButtonPressed(MouseButton mousebutton);
		static bool IsMouseButtonReleased(MouseButton mousebutton);

		static void SetMousePosition(const glm::vec2& value) { MousePosition = value; }
		static glm::vec2 GetMousePosition() { return MousePosition; }

		static void SetMouseOffset(const glm::vec2& value) { MouseOffset = value; }
		static glm::vec2 GetMouseOffset() { return MouseOffset; }

		static void SetMouseScroll(const glm::vec2& value) { MouseScroll = value; }
		static glm::vec2 GetMouseScroll() { return MouseScroll; }

		static void ResetInput();
	
	private:
		inline static glm::vec2 MousePosition;
		inline static glm::vec2 MouseOffset;
		inline static glm::vec2 MouseScroll;
	};
}
