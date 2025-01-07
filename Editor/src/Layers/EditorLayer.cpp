#include "EditorLayer.h"
#include <imgui.h>

void EditorLayer::OnStart()
{
    // Initialize the scene
    m_Scene = new PIX3D::Scene("IBL Scene");
    m_Scene->OnStart();

    // Initialize editor widgets
    m_LightningWidget = new LightningWidget(m_Scene);
    m_HierarchyWidget = new HierarchyWidget(m_Scene);
    m_InspectorWidget = new InspectorWidget(m_Scene, m_HierarchyWidget);
    m_MaterialWidget = new MaterialWidget(m_Scene, m_HierarchyWidget);

    auto& CurrentProj = Engine::GetCurrentProjectRef();
    int x = 0;
}

void EditorLayer::OnUpdate(float dt)
{
    // Update the scene
    m_Scene->OnUpdate(dt);

    // Handle input for mouse cursor visibility
    if (PIX3D::Input::IsKeyPressed(PIX3D::KeyCode::RightShift))
        ShowMouseCursor = false;
    else if (PIX3D::Input::IsKeyPressed(PIX3D::KeyCode::Escape))
        ShowMouseCursor = true;

    PIX3D::Engine::GetPlatformLayer()->ShowCursor(ShowMouseCursor);

    // Render the scene
    m_Scene->OnRender();

    // Render UI
    RenderMenuBar();
    RenderWidgets();
}

void EditorLayer::OnDestroy()
{
    delete m_LightningWidget;
    m_LightningWidget = nullptr;
    delete m_HierarchyWidget;
    m_HierarchyWidget = nullptr;
    delete m_InspectorWidget;
    m_InspectorWidget = nullptr;
    delete m_MaterialWidget;
    m_MaterialWidget = nullptr;
    delete m_Scene;
    m_Scene = nullptr;
}

void EditorLayer::OnKeyPressed(uint32_t key)
{
    PIX3D::KeyCode keycode = (PIX3D::KeyCode)key;

    if (keycode == PIX3D::KeyCode::LeftControl)
    {
        // Save Scene
        if (PIX3D::Input::IsKeyPressed(PIX3D::KeyCode::S))
        {
            SaveSceneDialogue();
        }

        // Load Scene
        if (PIX3D::Input::IsKeyPressed(PIX3D::KeyCode::D))
        {
            LoadSceneDialogue();
        }
    }
}

void EditorLayer::SaveSceneDialogue()
{
    auto* PlatformLayer = PIX3D::Engine::GetPlatformLayer();

    std::filesystem::path ScenePath = PlatformLayer->SaveDialogue(FileDialougeFilter::PIXSCENE);
    if (!ScenePath.string().empty())
    {
        SceneSerializer::SaveScene_Text(m_Scene, ScenePath);
    }
}

void EditorLayer::LoadSceneDialogue()
{
    auto* PlatformLayer = PIX3D::Engine::GetPlatformLayer();

    std::filesystem::path ScenePath = PlatformLayer->OpenDialogue(FileDialougeFilter::PIXSCENE);
    if (!ScenePath.string().empty() && ScenePath.extension().string() == ".pixscene")
    {
        SceneSerializer::LoadScene_Text(m_Scene, ScenePath);
    }
}

void EditorLayer::RenderMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Scene ...", "Cntrl + S"))
            {
                SaveSceneDialogue();
            }

            if (ImGui::MenuItem("Load Scene ...", "Cntrl + D"))
            {
                LoadSceneDialogue();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window"))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, m_ShowLightningWidget ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            if (ImGui::MenuItem("Lightning"))
            {
                m_ShowLightningWidget = !m_ShowLightningWidget;
            }
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Text, m_ShowHierarchyWidget ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            if (ImGui::MenuItem("Hierarchy"))
            {
                m_ShowHierarchyWidget = !m_ShowHierarchyWidget;
            }
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Text, m_ShowInspectorWidget ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            if (ImGui::MenuItem("Inspector"))
            {
                m_ShowInspectorWidget = !m_ShowInspectorWidget;
            }
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Text, m_ShowMaterialWidget ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            if (ImGui::MenuItem("Material"))
            {
                m_ShowMaterialWidget = !m_ShowMaterialWidget;
            }
            ImGui::PopStyleColor();

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void EditorLayer::RenderWidgets()
{
    if (m_ShowLightningWidget)
        m_LightningWidget->OnRender();
    if (m_ShowHierarchyWidget)
        m_HierarchyWidget->OnRender();
    if (m_ShowInspectorWidget)
        m_InspectorWidget->OnRender();
    if (m_ShowMaterialWidget)
        m_MaterialWidget->OnRender();
}
