#pragma once
#include "HierarchyWidget.h"

class InspectorWidget
{
public:
    InspectorWidget(Scene* scene, HierarchyWidget* hierarchy)
        : m_Scene(scene), m_HierarchyWidget(hierarchy) {}

    void OnRender();

private:
    Scene* m_Scene;
    HierarchyWidget* m_HierarchyWidget;
};
