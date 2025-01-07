#include "Input.h"
#include <Engine/Engine.hpp>

namespace PIX3D
{
    bool Input::IsKeyPressed(KeyCode keycode)
    {
        return Engine::GetPlatformLayer()->IsKeyPressed((int)keycode);
    }

    bool Input::IsKeyReleased(KeyCode keycode)
    {
        return Engine::GetPlatformLayer()->IsKeyReleased((int)keycode);
    }

    bool Input::IsMouseButtonPressed(MouseButton mousebutton)
    {
        return Engine::GetPlatformLayer()->IsMouseButtonPressed((int)mousebutton);
    }

    bool Input::IsMouseButtonReleased(MouseButton mousebutton)
    {
        return Engine::GetPlatformLayer()->IsMouseButtonReleased((int)mousebutton);
    }

    void Input::ResetInput()
    {
        MouseOffset = { 0.0f, 0.0f };
        MouseScroll = { 0.0f, 0.0f };
    }
}
