// MaterialWidget.h
#pragma once
#include "HierarchyWidget.h"

class MaterialWidget
{
public:
    MaterialWidget(Scene* scene, HierarchyWidget* hierarchy)
        : m_Scene(scene), m_HierarchyWidget(hierarchy) {
    }

    void OnRender();
    void PostFrameProcesses();

private:
    void DrawMaterialUI(StaticMeshComponent* meshComponent, PIX3D::VulkanBaseColorMaterial& material, size_t submeshIndex);

    PIX3D::Scene* m_Scene = nullptr;
    HierarchyWidget* m_HierarchyWidget = nullptr;

    // Texture loading operation tracking
    struct PendingTextureLoad {
        size_t submeshIndex;
        size_t materialIndex;
        enum class TextureType {
            Albedo,
            Normal,
            MetallicRoughness,
            AO,
            Emissive
        } type;
    };
    std::vector<PendingTextureLoad> m_PendingTextureLoads;
};
