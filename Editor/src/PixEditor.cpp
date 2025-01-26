#include "PixEditor.h"
#include <Platfrom/Vulkan/VulkanHelper.h>
#include <imgui.h>

#include "Layers/Blur Example/FrostedGlassExample.h"

void PixEditor::OnStart()
{
    m_LayerManager.init(new FrostedGlassExample());
    m_LayerManager.OnStart();
}

void PixEditor::OnUpdate(float dt)
{
    m_LayerManager.OnUpdate(dt);
}

void PixEditor::OnDestroy()
{
}

void PixEditor::OnResize(uint32_t width, uint32_t height)
{
}

void PixEditor::OnKeyPressed(uint32_t key)
{
    m_LayerManager.OnKeyPressed(key);
}
