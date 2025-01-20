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

    bool m_SpriteTextureDropped = false;
    bool m_SpriteAnimatorTextureDropped = false;
    PIX3D::UUID m_DroppedTextureUUID;
};
