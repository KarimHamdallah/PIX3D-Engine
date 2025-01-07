#pragma once

#include "../Editor Widgets/LightningWidget.h"
#include "../Editor Widgets/InspectorWidget.h"
#include "../Editor Widgets/MaterialWidget.h"
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

    bool m_ShowLightningWidget = false;
    bool m_ShowHierarchyWidget = false;
    bool m_ShowInspectorWidget = false;
    bool m_ShowMaterialWidget = false;

    PIX3D::Scene* m_Scene = nullptr;
    LightningWidget* m_LightningWidget = nullptr;
    HierarchyWidget* m_HierarchyWidget = nullptr;
    InspectorWidget* m_InspectorWidget = nullptr;
    MaterialWidget* m_MaterialWidget = nullptr;

    bool ShowMouseCursor = true;
};
