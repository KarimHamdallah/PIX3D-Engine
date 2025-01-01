#include "Input.h"
#include <Engine/Engine.hpp>

namespace PIX3D
{
    /// <summary>
    /// Checks whether a specific keyboard key is currently pressed down.
    /// </summary>
    /// <param name="keycode">The key identifier (see KeyCode enum).</param>
    /// <returns>true if the key is pressed; otherwise, false.</returns>
    bool Input::IsKeyPressed(KeyCode keycode)
    {
        // Extra error check or logging can be added here if needed
        return Engine::GetPlatformLayer()->IsKeyPressed(static_cast<int>(keycode));
    }

    /// <summary>
    /// Checks whether a specific keyboard key was released this frame.
    /// </summary>
    /// <param name="keycode">The key identifier (see KeyCode enum).</param>
    /// <returns>true if the key was released this frame; otherwise, false.</returns>
    bool Input::IsKeyReleased(KeyCode keycode)
    {
        return Engine::GetPlatformLayer()->IsKeyReleased(static_cast<int>(keycode));
    }

    /// <summary>
    /// Checks if a specific mouse button is currently pressed.
    /// </summary>
    /// <param name="mousebutton">The mouse button identifier (see MouseButton enum).</param>
    /// <returns>true if the mouse button is pressed; otherwise, false.</returns>
    bool Input::IsMouseButtonPressed(MouseButton mousebutton)
    {
        return Engine::GetPlatformLayer()->IsMouseButtonPressed(static_cast<int>(mousebutton));
    }

    /// <summary>
    /// Checks if a specific mouse button was released this frame.
    /// </summary>
    /// <param name="mousebutton">The mouse button identifier (see MouseButton enum).</param>
    /// <returns>true if the mouse button was released; otherwise, false.</returns>
    bool Input::IsMouseButtonReleased(MouseButton mousebutton)
    {
        return Engine::GetPlatformLayer()->IsMouseButtonReleased(static_cast<int>(mousebutton));
    }

    /// <summary>
    /// Resets the mouse offset and scroll values to zero, typically called once per frame
    /// to clear out old state.
    /// </summary>
    void Input::ResetInput()
    {
        MouseOffset = { 0.0f, 0.0f };
        MouseScroll = { 0.0f, 0.0f };
    }
}
