#pragma once

#include "../Editor Widgets/LightningWidget.h"
#include "../Editor Widgets/InspectorWidget.h"
#include "../Editor Widgets/MaterialWidget.h"
#include "../Editor Widgets/AssetWidget.h"
#include "../Layers/LauncherLayer.h"

class EditorLayer : public Layer
{
public:
    virtual void OnStart() override;
    virtual void OnUpdate(float dt) override;
    virtual void OnDestroy() override;
    virtual void OnKeyPressed(uint32_t key) override;

private:
    void SaveSceneDialogue();
    void LoadSceneDialogue();

private:
    void RenderMenuBar();
    void RenderWidgets();
    void RenderToolbar();

    bool m_ShowLightningWidget = false;
    bool m_ShowHierarchyWidget = false;
    bool m_ShowInspectorWidget = false;
    bool m_ShowMaterialWidget = false;
    bool m_ShowAssetWidget = false;

    PIX3D::Scene* m_Scene = nullptr;
    PIX3D::Scene* m_TempScene = nullptr;

    LightningWidget* m_LightningWidget = nullptr;
    HierarchyWidget* m_HierarchyWidget = nullptr;
    InspectorWidget* m_InspectorWidget = nullptr;
    MaterialWidget* m_MaterialWidget = nullptr;
    AssetWidget* m_AssetWidget = nullptr;

    bool ShowMouseCursor = true;



    ////////// Run Time ////////////
    VK::VulkanTexture* m_PlayIcon = nullptr;
    VK::VulkanTexture* m_StopIcon = nullptr;
    bool m_IsPlaying = false;

    //////// Grass ////////////
    uint32_t GrassCountX = 200;
    uint32_t GrassCountZ = 200;

    VK::VulkanTexture* m_TerrainGrassTexture = nullptr;
    VK::VulkanTexture* m_GrassTexture = nullptr;
    std::vector<glm::mat4> m_GrassTransformationMatrices;
    float m_Time = 0.0f;
    float m_WindStrength = 0.1f;
    glm::vec2 m_WindDirection = { 1.0f, 0.0f };
};
