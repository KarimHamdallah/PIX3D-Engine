#include "HierarchyWidget.h"
#include <imgui.h>

void HierarchyWidget::OnRender()
{
    ImGui::Begin("Hierarchy");

    // Create entity button
    if (ImGui::BeginPopupContextWindow("Create Entity", ImGuiPopupFlags_MouseButtonRight))
    {
        if (ImGui::MenuItem("Add Static Mesh"))
        {
            auto* platform = PIX3D::Engine::GetPlatformLayer();
            std::filesystem::path filepath = platform->OpenDialogue(PIX3D::FileDialougeFilter::GLTF);
            if (!filepath.empty() && (filepath.extension().string() == ".gltf" || filepath.extension().string() == ".glb" || filepath.extension().string() == ".obj"))
            {
                StaticMesh mesh;
                mesh.Load(filepath.string(), 1.0f);
                Transform transform;
                m_Scene->AddStaticMesh("New Mesh", transform, mesh);
            }
        }

        if (ImGui::MenuItem("Add Sprite"))
        {
            Transform transform;
            Sprite sprite;
            m_Scene->AddSprite("New Sprite", transform, sprite);
        }

        if (ImGui::MenuItem("Add Point Light"))
        {
            Transform transform;
            m_Scene->AddPointLight("New Point Light", transform, {1.0f, 1.0f, 1.0f, 1.0f});
        }

        if (ImGui::MenuItem("Add Dir Light"))
        {
            Transform transform;
            m_Scene->AddDirectionalLight("Dir Light", transform, { 1.0f, 1.0f, 1.0f, 1.0f });
        }

        if (ImGui::MenuItem("Add Sprite Animation"))
        {
            Transform transform;
            GL::GLTexture spriteSheet;
            auto* platform = PIX3D::Engine::GetPlatformLayer();
            std::filesystem::path filepath = platform->OpenDialogue(PIX3D::FileDialougeFilter::PNG);
            if (!filepath.empty())
            {
                spriteSheet.LoadFromFile(filepath.string(), true);
                m_Scene->AddSpriteAnimation("New Animation", transform, spriteSheet, 4, 0.1f); // Default 4 frames at 0.1s each
            }
        }
        ImGui::EndPopup();
    }

    // Display entities
    auto view = m_Scene->m_Registry.view<TagComponent>();
    for (auto entity : view)
    {
        const auto& tag = view.get<TagComponent>(entity).m_Tag;

        ImGuiTreeNodeFlags flags = (m_SelectedEntity == entity ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf;

        bool opened = ImGui::TreeNodeEx((void*)(uint64_t)entity, flags, tag.c_str());
        if (ImGui::IsItemClicked())
        {
            m_SelectedEntity = entity;
        }

        if (opened)
            ImGui::TreePop();
    }

    ImGui::End();
}
