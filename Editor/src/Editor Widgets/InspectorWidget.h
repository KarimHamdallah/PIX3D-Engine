#pragma once
#include "HierarchyWidget.h"
#include <imgui.h>

class InspectorWidget
{
public:
    InspectorWidget(Scene* scene, HierarchyWidget* hierarchy)
        : m_Scene(scene), m_HierarchyWidget(hierarchy) {}

    void OnRender();

    void PostFrameProcesses();
private:
    Scene* m_Scene;
    HierarchyWidget* m_HierarchyWidget;
    std::vector<VK::VulkanTexture*> m_TexturesToFree;


    bool SpriteAnimatorComponentChangeTexture = false;
    bool SpriteComponentChangeTexture = false;
};
