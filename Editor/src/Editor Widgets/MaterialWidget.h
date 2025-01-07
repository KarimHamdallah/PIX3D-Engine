#pragma once
#include "HierarchyWidget.h"

class MaterialWidget
{
public:
    MaterialWidget(Scene* scene, HierarchyWidget* hierarchy)
        : m_Scene(scene), m_HierarchyWidget(hierarchy) {}

    void OnRender();

private:
    void DrawMaterialUI(PIX3D::BaseColorMaterial& material);

    PIX3D::Scene* m_Scene = nullptr;
    HierarchyWidget* m_HierarchyWidget;
};
